#include "eval_cost.h"
#include "../asm.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace rspl {

int evalFunctionCost(AsmFunc &func) {
  // Filter to only OP instructions (including NOPs)
  std::vector<AsmInst *> ops;
  for (auto &inst : func.asm_) {
    if (inst.type == AsmType::OP) {
      ops.push_back(&inst);
    }
  }
  if (ops.empty()) return 0;

  int regCycleMap[64] = {};
  int cycle = 0;
  int pc = 0;
  uint64_t lastLoadPosMask = 0;
  int execCount = 0;

  auto ticks = [&](int count) {
    for (int i = 0; i < 64; ++i) regCycleMap[i] -= count;
    lastLoadPosMask >>= count;
    cycle += count;
  };

  // Branch state: 0=none, 2=branch, 1=delay
  int branchStep = 0;
  bool didJump = false;

  while (pc < (int)ops.size()) {
    // Resolve stalls for to-be-executed instructions
    int lastCycle;
    do {
      lastCycle = cycle;
      for (int i = 0; i < execCount && pc + i < (int)ops.size(); ++i) {
        AsmInst *execOp = ops[pc + i];
        execOp->debug.paired = (execCount == 2);

        for (int src : execOp->depsStallSourceIdx) {
          if (regCycleMap[src] > 0) ticks(regCycleMap[src]);
        }
        if ((lastLoadPosMask & 0b001) &&
            (execOp->opFlags & OpFlag::OP_FLAG_IS_MEM_STALL_STORE)) {
          execOp->debug.stall++;
          ticks(1);
        }
      }
    } while (lastCycle != cycle);

    // Execute
    for (int i = 0; i < execCount && pc + i < (int)ops.size(); ++i) {
      AsmInst *execOp = ops[pc + i];
      if (execOp->opFlags & OpFlag::OP_FLAG_IS_MEM_STALL_LOAD)
        lastLoadPosMask |= 0b100;
      didJump |= (execOp->opFlags & OpFlag::OP_FLAG_LIKELY_BRANCH);

      branchStep >>= 1;
      if (!branchStep && (execOp->opFlags & OpFlag::OP_FLAG_IS_BRANCH))
        branchStep = 2; // BRANCH_STEP_BRANCH

      if (didJump && branchStep == 1) { // BRANCH_STEP_DELAY
        ticks(1);
        didJump = false;
      }

      execOp->debug.cycle = cycle;
      for (int dst : execOp->depsStallTargetIdx) {
        regCycleMap[dst] = execOp->stallLatency;
      }
    }

    pc += execCount;
    if (pc >= (int)ops.size()) break;

    AsmInst *op = ops[pc];
    AsmInst *opNext = (pc + 1 < (int)ops.size()) ? ops[pc + 1] : nullptr;

    bool canDualIssue =
        opNext &&
        ((op->opFlags & OpFlag::OP_FLAG_IS_VECTOR) !=
         (opNext->opFlags & OpFlag::OP_FLAG_IS_VECTOR)) &&
        !(branchStep == 2) && !(op->opFlags & OpFlag::OP_FLAG_IS_BRANCH) &&
        (op->depsStallTargetMask0 & opNext->depsStallSourceMask0) == 0 &&
        (op->depsStallTargetMask1 & opNext->depsStallSourceMask1) == 0 &&
        (op->depsStallTargetMask0 & opNext->depsStallTargetMask0) == 0 &&
        (op->depsStallTargetMask1 & opNext->depsStallTargetMask1) == 0;

    // CFC2/CTC2: prevent dual-issue if the current instruction writes
    // to any register that the CFC2/CTC2 reads or writes (incl. ctrl regs).
    if (canDualIssue && (opNext->opFlags & OpFlag::OP_FLAG_CTC2_CFC2)) {
      for (int i = 0; i < 5; ++i) {
        if ((op->depsTargetMask[i] & opNext->depsSourceMask[i]) ||
            (op->depsTargetMask[i] & opNext->depsTargetMask[i])) {
          canDualIssue = false;
          break;
        }
      }
    }

    execCount = canDualIssue ? 2 : 1;
    ticks(1);
  }
  return cycle;
}

} // namespace rspl

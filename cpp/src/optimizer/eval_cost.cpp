#include "eval_cost.h"
#include "../asm.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace rspl {

int evalFunctionCost(AsmFunc &func) {
  // Filter to only OP instructions (including NOPs).
  // Thread-local to reuse allocation across evaluations.
  static thread_local std::vector<AsmInst *> ops;
  ops.clear();
  for (auto &inst : func.asm_) {
    if (inst.type == AsmType::OP) {
      ops.push_back(&inst);
    }
  }
  if (ops.empty()) return 0;

  // regStallExpiry[r] = cycle when register r's stall expires.
  // 0 means no active stall (cycle starts at 0, so expiry > 0 means active).
  // Replaces the old regCycleMap[64] which stored remaining cycles and
  // was fully decremented on every tick (O(64*cycles) → O(active_stalls)).
  int regStallExpiry[64] = {};
  int cycle = 0;
  int pc = 0;
  uint64_t lastLoadPosMask = 0;
  int execCount = 0;

  // Branch state: 0=none, 2=branch, 1=delay
  int branchStep = 0;
  bool didJump = false;

  while (pc < (int)ops.size()) {
    // Resolve stalls for to-be-executed instructions.
    // Find the max expiry among all source stall registers and advance
    // cycle there directly — no need to loop or decrement all 64 entries.
    bool resolved;
    do {
      resolved = true;
      for (int i = 0; i < execCount && pc + i < (int)ops.size(); ++i) {
        AsmInst *execOp = ops[pc + i];
        execOp->debug.paired = (execCount == 2);

        for (int src : execOp->depsStallSourceIdx) {
          if (regStallExpiry[src] > cycle) {
            int advance = regStallExpiry[src] - cycle;
            cycle += advance;
            lastLoadPosMask >>= advance;
            resolved = false;
          }
        }
        if ((lastLoadPosMask & 0b001) &&
            (execOp->opFlags & OpFlag::OP_FLAG_IS_MEM_STALL_STORE)) {
          execOp->debug.stall++;
          cycle += 1;
          lastLoadPosMask >>= 1;
          resolved = false;
        }
      }
    } while (!resolved);

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
        cycle += 1;
        lastLoadPosMask >>= 1;
        didJump = false;
      }

      execOp->debug.cycle = cycle;
      for (int dst : execOp->depsStallTargetIdx) {
        regStallExpiry[dst] = cycle + execOp->stallLatency;
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
    cycle += 1;
    lastLoadPosMask >>= 1;
  }
  return cycle;
}

} // namespace rspl
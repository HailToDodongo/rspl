#include "asm_optimizer.h"
#include "asm.h"
#include "asm_scan_deps.h"
#include "eval_cost.h"
#include "patterns/assertCompare.h"
#include "patterns/branchJump.h"
#include "patterns/commandAlias.h"
#include "patterns/dedupeImm.h"
#include "patterns/dedupeJumps.h"
#include "patterns/dedupeLabels.h"
#include "patterns/mergeSequence.h"
#include "patterns/removeDeadCode.h"
#include "patterns/tailCall.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <random>

namespace rspl {

// --- PRNG (matches JS LCG) --------------------------------------------

static uint32_t seed = 0x41C64E6D;

static void setSeed(uint32_t s) { seed = s; }

static double rand01() {
  seed = (seed * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
  return (seed >> 16) / 65536.0;
}

static int randIndex(int maxExcl) {
  seed = (seed * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
  return (seed >> 16) % maxExcl;
}

// --- Pattern optimization runner --------------------------------------

void asmOptimizePattern(AsmFunc &func) {
  dedupeLabels(func);
  dedupeJumps(func);
  branchJump(func);
  tailCall(func);
  dedupeImmediate(func);
  mergeSequence(func);
  assertCompare(func);
  removeDeadCode(func);
  commandAlias(func);
}

// --- Delay slot filling -----------------------------------------------

// Forward-declared from asm_scan_deps.cpp
std::vector<int> asmGetReorderIndices(const std::vector<AsmInst> &asmList,
                                      int i);

void fillDelaySlots(AsmFunc &func) {
  // Process right-to-left so instructions closest to the NOP get first
  // chance at filling it.  This matters when multiple instructions are
  // candidates for the same delay slot.
  for (int i = static_cast<int>(func.asm_.size()) - 1; i >= 0; --i) {
    auto &inst = func.asm_[i];
    if (inst.type != AsmType::OP) continue;
    if (inst.opFlags &
        (OpFlag::OP_FLAG_IS_IMMOVABLE | OpFlag::OP_FLAG_IS_NOP |
         OpFlag::OP_FLAG_IS_BRANCH))
      continue;

    auto reorderRange = asmGetReorderIndices(func.asm_, i);

    int delaySlotIdx = -1;
    for (int idx : reorderRange) {
      if (idx <= i) continue; // forward only
      if (func.asm_[idx].opFlags & OpFlag::OP_FLAG_IS_NOP) {
        delaySlotIdx = idx;
        break;
      }
    }

    if (delaySlotIdx >= 0) {
      func.asm_[delaySlotIdx] = std::move(inst);
      func.asm_[i] = asmNOP();
    }
  }
}

// --- Reorder optimization (stochastic annealing) ----------------------

static void relocateElement(std::vector<AsmInst> &arr, int from, int to) {
  if (from == to) return;
  if (arr[to].opFlags & OpFlag::OP_FLAG_IS_BRANCH) return;
  bool targetIsNOP = arr[to].opFlags & OpFlag::OP_FLAG_IS_NOP;
  bool sourceInDelaySlot =
      (from >= 1) && (arr[from - 1].opFlags & OpFlag::OP_FLAG_IS_BRANCH);

  if (sourceInDelaySlot) {
    if (targetIsNOP) {
      std::swap(arr[to], arr[from]);
    } else {
      AsmInst inst = std::move(arr[from]);
      arr[from] = asmNOP();
      asmInitDep(arr[from]);
      arr.insert(arr.begin() + to, std::move(inst));
    }
  } else {
    if (targetIsNOP) {
      std::swap(arr[to], arr[from]);
      arr.erase(arr.begin() + from);
    } else {
      AsmInst inst = std::move(arr[from]);
      arr.erase(arr.begin() + from);
      arr.insert(arr.begin() + to, std::move(inst));
    }
  }
}

static int optimizeStep(AsmFunc &func) {
  if ((int)func.asm_.size() < 2) return 0;

  int i;
  std::vector<int> reorderIndices;
  for (int r = 0; r < 50; ++r) {
    i = randIndex(func.asm_.size());
    reorderIndices = asmGetReorderIndices(func.asm_, i);
    if (reorderIndices.size() > 1) break;
  }
  if (reorderIndices.size() <= 1) return 0;

  int targetIdx = i;
  if (rand01() < 0.80) {
    for (int j : reorderIndices) {
      if ((func.asm_[j].opFlags & OpFlag::OP_FLAG_IS_VECTOR) !=
          (func.asm_[i].opFlags & OpFlag::OP_FLAG_IS_VECTOR)) {
        if (!func.asm_[j].debug.paired) {
          targetIdx = j;
          break;
        }
      }
    }
  }

  if (targetIdx == i) {
    while (targetIdx == i) {
      targetIdx = reorderIndices[randIndex(reorderIndices.size())];
    }
  }

  func.asm_[i].debug.reorderCount++;
  relocateElement(func.asm_, i, targetIdx);
  asmInitDeps(func);
  return 1;
}

void asmOptimize(AsmFunc &func, int maxTimeMs) {
  asmInitDeps(func);
  int costBest = evalFunctionCost(func);
  int costInit = costBest;

  auto startTime = std::chrono::steady_clock::now();
  auto deadline = startTime + std::chrono::milliseconds(maxTimeMs);

  int iterations = 0;
  int stepsSinceLastOpt = 0;

  while (true) {
    auto now = std::chrono::steady_clock::now();
    if (now > deadline) break;

    auto asmCopy = func.asm_;

    for (int s = 0; s < 8; ++s) {
      optimizeStep(func);
    }

    asmInitDeps(func);
    int newCost = evalFunctionCost(func);

    if (newCost < costBest) {
      costBest = newCost;
      stepsSinceLastOpt = 0;
    } else if (newCost > costBest || rand01() < 0.5) {
      func.asm_ = std::move(asmCopy);
      ++stepsSinceLastOpt;
    } else {
      ++stepsSinceLastOpt;
    }

    ++iterations;
    if (stepsSinceLastOpt > 2000) break;
  }

  func.cyclesBefore = costInit;
  func.cyclesAfter = costBest;
}

} // namespace rspl

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
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>

namespace rspl {

// --- PRNG (matches JS LCG) --------------------------------------------

static uint32_t seed_ = 0x41C64E6D;

static void setSeed(uint32_t s) { seed_ = s; }

static double rand01() {
  seed_ = (seed_ * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
  return (seed_ >> 16) / 65536.0;
}

static int randIndex(int maxExcl) {
  seed_ = (seed_ * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
  return (seed_ >> 16) % maxExcl;
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

void fillDelaySlots(AsmFunc &func) {
  for (size_t i = 0; i < func.asm_.size(); ++i) {
    auto &inst = func.asm_[i];
    if (inst.type != AsmType::OP) continue;
    if (inst.opFlags &
        (OpFlag::OP_FLAG_IS_IMMOVABLE | OpFlag::OP_FLAG_IS_NOP |
         OpFlag::OP_FLAG_IS_BRANCH))
      continue;

    auto reorderRange = asmGetReorderIndices(func.asm_, static_cast<int>(i));

    int delaySlotIdx = -1;
    for (int idx : reorderRange) {
      if (idx <= static_cast<int>(i)) continue;
      if (func.asm_[idx].opFlags & OpFlag::OP_FLAG_IS_NOP) {
        delaySlotIdx = idx;
        break;
      }
    }

    if (delaySlotIdx >= 0) {
      func.asm_[delaySlotIdx] = std::move(inst);
      func.asm_.erase(func.asm_.begin() + static_cast<long>(i));
      --i;
    }
  }
}

// ==========================================================================
// Reorder optimization (stochastic annealing, matches JS algorithm)
// ==========================================================================

// --- Constants (matching JS) ----------------------------------------------

constexpr int POOL_SIZE = 8;
constexpr double PREFER_STALLS_RATE = 0.20;
constexpr double PREFER_PAIR_RATE = 0.80;
constexpr int MAX_STEPS_NO_CHANGE = 5000;
constexpr int SEARCH_VARIANT_SEARCH = 10;
constexpr int SEARCH_BACK_STEPS_FACTOR = 10;
constexpr int SEARCH_FWD_STEPS_FACTOR = 5;
constexpr int REORDER_MIN_OPS = 3;
constexpr int REORDER_MAX_OPS = 15;

// --- Helper functions -----------------------------------------------------

static AsmFunc cloneFunction(const AsmFunc &func) {
  AsmFunc copy = func;
  copy.asm_ = func.asm_; // vector copy
  return copy;
}

// Forward-declared
static std::pair<AsmFunc, int> generateWorseFunction(const AsmFunc &base,
                                                     int steps);

// --- relocateElement (matches JS relocateElement) --------------------------

static void relocateElement(std::vector<AsmInst> &arr, int from, int to) {
  if (from == to) return;
  if (arr[to].opFlags & OpFlag::OP_FLAG_IS_BRANCH) return;
  bool targetIsNOP = arr[to].opFlags & OpFlag::OP_FLAG_IS_NOP;
  bool sourceInDelaySlot =
      (from >= 1) && (arr[from - 1].opFlags & OpFlag::OP_FLAG_IS_BRANCH);

  if (sourceInDelaySlot) {
    if (targetIsNOP) {
      // Replace NOP with delay-slot instruction (keep delay slot filled)
      arr[to] = arr[from];
    } else {
      AsmInst inst = std::move(arr[from]);
      arr[from] = asmNOP();
      asmInitDep(arr[from]);
      arr.insert(arr.begin() + to, std::move(inst));
    }
  } else {
    if (targetIsNOP) {
      arr[to] = std::move(arr[from]);
      arr.erase(arr.begin() + from);
    } else {
      AsmInst inst = std::move(arr[from]);
      arr.erase(arr.begin() + from);
      if (to > from) to--;
      arr.insert(arr.begin() + to, std::move(inst));
    }
  }
}

// --- optimizeStep (matches JS optimizeStep) --------------------------------

static int optimizeStep(AsmFunc &func) {
  auto sz = static_cast<int>(func.asm_.size());
  if (sz < 2) return 0;

  int i = 0;
  std::vector<int> reorderIndices;
  for (int r = 0; r < 50; ++r) {
    i = randIndex(sz);
    reorderIndices = asmGetReorderIndices(func.asm_, i);
    if ((int)reorderIndices.size() > 1) break;
  }
  if ((int)reorderIndices.size() <= 1) return 0;

  int targetIdx = i;
  bool foundIndex = false;

  // Prefer pairing opposite-type (vector<->scalar) unpaired instructions
  if (rand01() < PREFER_PAIR_RATE) {
    for (int j : reorderIndices) {
      if ((func.asm_[j].opFlags & OpFlag::OP_FLAG_IS_VECTOR) !=
          (func.asm_[i].opFlags & OpFlag::OP_FLAG_IS_VECTOR)) {
        if (!func.asm_[j].debug.paired) {
          targetIdx = j;
          foundIndex = true;
        }
      }
    }
    if (!foundIndex) return 0;
  }

  // Prefer filling high-stall positions
  if (!foundIndex && rand01() < PREFER_STALLS_RATE) {
    int maxStalls = 0;
    for (int j : reorderIndices) {
      int stalls = func.asm_[j].debug.stall;
      if (stalls > maxStalls) {
        maxStalls = stalls;
        targetIdx = j;
        foundIndex = true;
      }
    }
  }

  if (!foundIndex) {
    while (targetIdx == i) {
      targetIdx = reorderIndices[randIndex((int)reorderIndices.size())];
    }
  }

  func.asm_[i].debug.reorderCount++;
  relocateElement(func.asm_, i, targetIdx);
  return 1;
}

// --- reorderRound (matches JS reorderRound) --------------------------------

struct RoundResult {
  int cost;
  std::vector<AsmInst> asm_;
};

static RoundResult reorderRound(const AsmFunc &baseFunc) {
  AsmFunc func = cloneFunction(baseFunc);
  int opCount = randIndex(REORDER_MAX_OPS - REORDER_MIN_OPS) + REORDER_MIN_OPS;
  for (int o = 0; o < opCount; ++o) {
    optimizeStep(func);
  }
  asmInitDeps(func);
  int cost = evalFunctionCost(func);
  return {cost, std::move(func.asm_)};
}

// --- generateWorseFunction (matches JS generateWorseFunction) ---------------

static std::pair<AsmFunc, int> generateWorseFunction(const AsmFunc &base,
                                                     int steps) {
  int maxCost = 0;
  AsmFunc newWorst = cloneFunction(base);
  for (int i = 0; i < steps; ++i) {
    AsmFunc f = cloneFunction(base);
    reorderRound(f);
    reorderRound(f);
    asmInitDeps(f);
    int cost = evalFunctionCost(f);
    if (cost > maxCost) {
      newWorst = std::move(f);
      maxCost = cost;
    }
  }
  return {std::move(newWorst), maxCost};
}

// --- Helper: format time string from ms -----------------------------------

static std::string formatTimeMs(int ms) {
  int h = ms / 3600000;
  int m = (ms % 3600000) / 60000;
  int s = (ms % 60000) / 1000;
  std::ostringstream ss;
  ss << std::setfill('0') << std::setw(2) << h << ":";
  ss << std::setfill('0') << std::setw(2) << m << ":";
  ss << std::setfill('0') << std::setw(2) << s;
  return ss.str();
}

// --- asmOptimize (matches JS asmOptimize) ----------------------------------

void asmOptimize(AsmFunc &func, int maxTimeMs) {
  const std::string &funcName =
      func.name.empty() ? "(???)" : func.name;

  asmInitDeps(func);
  int costBest = evalFunctionCost(func);
  func.cyclesBefore = costBest;
  int costInit = costBest;

  std::cerr << "Starting optimization with max. time: "
            << formatTimeMs(maxTimeMs) << std::endl;

  // Initialize random seed from system entropy (JS uses Math.random)
  std::random_device rd;
  setSeed(rd());

  AsmFunc lastRandPick = cloneFunction(func);

  auto startTime = std::chrono::steady_clock::now();
  auto deadline = startTime + std::chrono::milliseconds(maxTimeMs);

  int i = 0;
  int stepsSinceLastOpt = 0;
  int consecutiveSame = 0;
  double totalTime = 0.0;
  auto iterStart = startTime;

  while (totalTime < maxTimeMs) {
    auto now = std::chrono::steady_clock::now();

    // Progress logging every 1000 iterations
    if (i != 0 && (i % 1000) == 0) {
      auto dur = std::chrono::duration<double, std::milli>(now - iterStart)
                     .count();
      totalTime += dur;
      double left = maxTimeMs - totalTime;
      std::cerr << "[" << funcName << "] Step: " << i
                << ", Left: " << std::fixed << std::setprecision(4) << left
                << "ms | Time: " << dur << "ms" << std::endl;
      iterStart = now;
    }

    // Check timeout
    if (now > deadline) {
      std::cerr << "[" << funcName << "] Timeout after " << i
                << " iterations." << std::endl;
      break;
    }

    AsmFunc funcCopy = cloneFunction(func);
    std::vector<RoundResult> results;

    if (stepsSinceLastOpt > MAX_STEPS_NO_CHANGE) {
      ++consecutiveSame;
      int stepsBack = consecutiveSame * SEARCH_BACK_STEPS_FACTOR;
      int stepsFwd = consecutiveSame * SEARCH_FWD_STEPS_FACTOR;
      std::cerr << "[" << funcName << "] " << stepsSinceLastOpt
                << " steps since last improvement, generate new versions ("
                << stepsBack << " steps backward)" << std::endl;

      // Escape local minimum: generate worse variants then improve them
      for (int s = 0; s < SEARCH_VARIANT_SEARCH; ++s) {
        auto [worseCopy, maxCost] =
            generateWorseFunction(funcCopy, stepsBack);
        for (int t = 0; t < stepsFwd; ++t) {
          AsmFunc worseCopyTry = cloneFunction(worseCopy);
          reorderRound(worseCopyTry);
          asmInitDeps(worseCopyTry);
          int cost = evalFunctionCost(worseCopyTry);
          if (cost < maxCost) {
            worseCopy.asm_ = std::move(worseCopyTry.asm_);
            maxCost = cost;
          }
        }
        results.push_back(reorderRound(worseCopy));
      }
      for (int s = SEARCH_VARIANT_SEARCH; s < POOL_SIZE; ++s) {
        results.push_back(
            reorderRound(rand01() < 0.1 ? lastRandPick : funcCopy));
      }
      stepsSinceLastOpt = 0;
    } else {
      for (int s = 0; s < POOL_SIZE; ++s) {
        results.push_back(
            reorderRound(rand01() < 0.1 ? lastRandPick : funcCopy));
      }
    }

    for (int s = 0; s < (int)results.size(); ++s) {
      const auto &[cost, asm_] = results[s];
      bool isBetter = cost < costBest;
      bool isSame = cost == costBest;
      bool canUseTheSame = s < ((int)results.size() / 4);

      if (isBetter || (canUseTheSame && isSame)) {
        costBest = cost;
        func.asm_ = asm_;
        func.cyclesAfter = cost;

        if (isBetter) {
          std::cerr << "[" << funcName << "] \033[32m**** New Best for '"
                    << funcName << "': " << costInit << " -> " << cost
                    << " ****\033[0m" << std::endl;
          stepsSinceLastOpt = 0;
          consecutiveSame = 0;
        }
      }
    }

    if (i % 3 == 0) lastRandPick = funcCopy;
    ++i;
    ++stepsSinceLastOpt;
  }

  func.cyclesBefore = costInit;
  func.cyclesAfter = costBest;
}

} // namespace rspl
#include "asm_optimizer.h"
#include "asm_scan_deps.h"
#include "eval_cost.h"

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

// --- Pattern: dedupe labels -------------------------------------------

static void dedupeLabels(AsmFunc &func) {
  // Remove labels that point to the next instruction (redundant)
  for (size_t i = 0; i + 1 < func.asm_.size(); ++i) {
    if (func.asm_[i].type == AsmType::LABEL &&
        func.asm_[i + 1].type == AsmType::LABEL) {
      // Two labels in a row — merge by updating references
      std::string from = func.asm_[i + 1].label;
      std::string to = func.asm_[i].label;
      for (auto &inst : func.asm_) {
        if (inst.labelEnd == from) inst.labelEnd = to;
        for (auto &arg : inst.args) {
          if (arg == from) arg = to;
        }
      }
      func.asm_.erase(func.asm_.begin() + i + 1);
      --i; // recheck
    }
  }
}

// --- Pattern: dedupe jumps --------------------------------------------

static void dedupeJumps(AsmFunc &func) {
  // Remove j instructions that jump to the immediately following label
  for (size_t i = 0; i + 2 < func.asm_.size(); ++i) {
    if (func.asm_[i].op == "j" && func.asm_[i].args.size() == 1 &&
        func.asm_[i + 1].type == AsmType::OP &&
        func.asm_[i + 1].op == "nop" &&
        func.asm_[i + 2].type == AsmType::LABEL &&
        func.asm_[i + 2].label == func.asm_[i].args[0]) {
      func.asm_.erase(func.asm_.begin() + i, func.asm_.begin() + i + 2);
      --i;
    }
  }
}

// --- Pattern: remove dead code ----------------------------------------

static void removeDeadCode(AsmFunc &func) {
  // Remove unreachable code after unconditional jumps
  for (size_t i = 1; i < func.asm_.size(); ++i) {
    if (i >= 2 && func.asm_[i - 2].op == "j" &&
        func.asm_[i - 1].type == AsmType::OP &&
        func.asm_[i - 1].op == "nop" &&
        func.asm_[i].type != AsmType::LABEL) {
      // Remove until next label
      size_t end = i;
      while (end < func.asm_.size() &&
             func.asm_[end].type != AsmType::LABEL)
        ++end;
      if (end > i) {
        func.asm_.erase(func.asm_.begin() + i, func.asm_.begin() + end);
      }
    }
  }
}

// --- Pattern: dedupe immediate ----------------------------------------

static void dedupeImmediate(AsmFunc &func) {
  // Remove redundant `or $reg, $zero, $zero` before an `addiu $reg, $zero, 0`
  for (size_t i = 0; i + 1 < func.asm_.size(); ++i) {
    if (func.asm_[i].op == "or" && func.asm_[i].args.size() == 3 &&
        func.asm_[i].args[1] == "$zero" &&
        func.asm_[i].args[2] == "$zero") {
      std::string reg = func.asm_[i].args[0];
      if (func.asm_[i + 1].op == "addiu" &&
          func.asm_[i + 1].args.size() >= 2 &&
          func.asm_[i + 1].args[0] == reg &&
          func.asm_[i + 1].args[1] == "$zero") {
        // The addiu after or-zero is a load-immediate — keep just addiu
        func.asm_.erase(func.asm_.begin() + i);
        --i;
      }
    }
  }
}

// --- Pattern: branch jump ---------------------------------------------

static void branchJump(AsmFunc &func) {
  // Transform: beq $x, $y, LABEL -> nop -> j REAL_TARGET -> nop -> LABEL:
  // Into: bne $x, $y, REAL_TARGET (inverts condition, removes jump)
  for (size_t i = 0; i + 4 < func.asm_.size(); ++i) {
    auto &b = func.asm_[i];
    if (!(b.opFlags & OpFlag::OP_FLAG_IS_BRANCH) || b.labelEnd.empty())
      continue;
    if (b.op.starts_with("j")) continue; // not a cond branch
    if (func.asm_[i + 1].op != "nop") continue;
    if (func.asm_[i + 2].op != "j") continue;
    std::string realTarget = func.asm_[i + 2].args[0];
    if (func.asm_[i + 3].op != "nop") continue;
    if (func.asm_[i + 4].type != AsmType::LABEL) continue;
    if (func.asm_[i + 4].label != b.labelEnd) continue;

    // Invert branch and redirect
    b.labelEnd = realTarget;
    if (b.op == "beq") b.op = "bne";
    else if (b.op == "bne") b.op = "beq";
    else if (b.op == "bgez") b.op = "bltz";
    else if (b.op == "bltz") b.op = "bgez";
    else if (b.op == "blez") b.op = "bgtz";
    else if (b.op == "bgtz") b.op = "blez";
    else continue;

    // Remove j + nop + label
    func.asm_.erase(func.asm_.begin() + i + 2,
                    func.asm_.begin() + i + 5);
  }
}

// --- Pattern: tail call -----------------------------------------------

static void tailCall(AsmFunc &func) {
  // Convert: jal FOO -> nop -> jr $ra -> nop into j FOO
  for (size_t i = 0; i + 3 < func.asm_.size(); ++i) {
    if (func.asm_[i].op == "jal" && func.asm_[i + 1].op == "nop" &&
        func.asm_[i + 2].op == "jr" &&
        func.asm_[i + 2].args.size() >= 1 &&
        func.asm_[i + 2].args[0] == "$ra" &&
        func.asm_[i + 3].op == "nop") {
      func.asm_[i].op = "j";
      func.asm_.erase(func.asm_.begin() + i + 2,
                      func.asm_.begin() + i + 4);
    }
  }
}

// --- Pattern: merge sequence ------------------------------------------

static void mergeSequence(AsmFunc &func) {
  // Merge: addiu $x, $zero, N -> addu $y, $x, $z  into addiu $y, $z, N
  for (size_t i = 0; i + 1 < func.asm_.size(); ++i) {
    auto &a = func.asm_[i];
    auto &b = func.asm_[i + 1];
    if (a.op == "addiu" && a.args.size() >= 3 && a.args[1] == "$zero" &&
        b.op == "addu" && b.args.size() >= 3 &&
        b.args[1] == a.args[0]) {
      b.op = "addiu";
      b.args[1] = b.args[2];
      b.args[2] = a.args[2];
      func.asm_.erase(func.asm_.begin() + i);
      --i;
    }
  }
}

// --- Pattern: assert compare ------------------------------------------

static void assertCompare(AsmFunc &func) {
  // Pattern: bne $x, $y, SKIP -> nop -> lui $at, ERR -> j ASSERT -> nop -> SKIP:
  // Convert to: beq $x, $y, ASSERT -> (short assert)
  for (size_t i = 0; i + 5 < func.asm_.size(); ++i) {
    auto &b = func.asm_[i];
    if (!(b.opFlags & OpFlag::OP_FLAG_IS_BRANCH) || b.labelEnd.empty())
      continue;
    if (func.asm_[i + 1].op != "nop") continue;
    if (func.asm_[i + 2].op != "lui") continue;
    if (func.asm_[i + 3].op != "j") continue;
    if (func.asm_[i + 4].op != "nop") continue;
    if (func.asm_[i + 5].type != AsmType::LABEL ||
        func.asm_[i + 5].label != b.labelEnd)
      continue;

    // Shorten to use likely branch
    // (simplified — in real code this is more complex)
  }
}

// --- Pattern: command alias -------------------------------------------

static void commandAlias(AsmFunc &func) {
  if (func.asm_.size() < 2 || func.type != "command") return;

  auto &inst = func.asm_;
  // Check if the command is just a simple branch to another label/function
  std::string op0 = inst[0].op;
  bool isBranch = (op0 == "j" || op0 == "jr" || op0 == "beq" || op0 == "bne");
  if (isBranch && (inst[1].opFlags & OpFlag::OP_FLAG_IS_NOP)) {
    if (!inst[0].args.empty()) {
      func.nameOverride = inst[0].args[0];
    }
    if (inst.size() == 2) {
      inst.clear(); // empty function — just an alias
    }
  }
}

// --- Pattern optimization runner --------------------------------------

void asmOptimizePattern(AsmFunc &func) {
  dedupeLabels(func);
  dedupeJumps(func);
  branchJump(func);
  tailCall(func);
  dedupeImmediate(func);
  mergeSequence(func);
  removeDeadCode(func);
  commandAlias(func);
}

// --- Delay slot filling ------------------------------------------------

// Forward-declared from asm_scan_deps.cpp
std::vector<int> asmGetReorderIndices(const std::vector<AsmInst> &asmList,
                                      int i);

void fillDelaySlots(AsmFunc &func) {
  // Move movable instructions forward into NOP delay slots.
  // Ported from JS: asmOptimizer.js fillDelaySlots
  for (size_t i = 0; i < func.asm_.size(); ++i) {
    auto &inst = func.asm_[i];
    if (inst.type != AsmType::OP ||
        (inst.opFlags & OpFlag::OP_FLAG_IS_IMMOVABLE))
      continue;

    auto reorderRange = asmGetReorderIndices(func.asm_, static_cast<int>(i));

    int delaySlotIdx = -1;
    for (int idx : reorderRange) {
      if (func.asm_[idx].opFlags & OpFlag::OP_FLAG_IS_NOP) {
        delaySlotIdx = idx;
        break;
      }
    }

    if (delaySlotIdx >= 0) {
      func.asm_[delaySlotIdx] = std::move(inst);
      func.asm_.erase(func.asm_.begin() + static_cast<long>(i));
      --i; // reprocess current index since the next instruction shifted
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
  // Try to pair vector with scalar (dual-issue optimization)
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
  asmInitDeps(func); // Recompute dependencies
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

    // Clone function state
    auto asmCopy = func.asm_; // deep copy

    // Run a few reorder steps
    for (int s = 0; s < 8; ++s) {
      optimizeStep(func);
    }

    asmInitDeps(func);
    int newCost = evalFunctionCost(func);

    if (newCost < costBest) {
      costBest = newCost;
      stepsSinceLastOpt = 0;
    } else if (newCost > costBest || rand01() < 0.5) {
      // Revert to copy
      func.asm_ = std::move(asmCopy);
      ++stepsSinceLastOpt;
    } else {
      ++stepsSinceLastOpt;
    }

    ++iterations;
    if (stepsSinceLastOpt > 2000) break; // stuck
  }

  func.cyclesBefore = costInit;
  func.cyclesAfter = costBest;
}

} // namespace rspl

#include "eval_cost.h"
#include "../asm.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

// Dependency-mask bits (word 4) of the vector control registers:
// $vco = 288, $vcc = 289, $vce = 294 (see asm_scan_deps.cpp).
static constexpr uint64_t CTRL_REG_MASK4 =
    (1ULL << (288 - 256)) | (1ULL << (289 - 256)) | (1ULL << (294 - 256));

// --- Straight-line pipeline engine -----------------------------------------
// Evaluates `ops` as one linear instruction stream from a clean pipeline
// state, writes debug.cycle/stall/paired on every op and returns the total
// cycle count. This is the validated per-instruction model (dual-issue,
// stall latencies, load/store port conflicts, branch bubbles).

static int evalSequence(std::vector<AsmInst *> &ops) {
  if (ops.empty()) return 0;

  // regStallExpiry[r] = cycle when register r's stall expires.
  // 0 means no active stall (cycle starts at 0, so expiry > 0 means active).
  int regStallExpiry[64] = {};
  int cycle = 0;
  int pc = 0;
  uint64_t lastLoadPosMask = 0;
  int execCount = 0;

  // Branch state: 0=none, 2=branch, 1=delay
  int branchStep = 0;
  bool didJump = false;
  bool inUncondJump = false;

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
      if (!branchStep && (execOp->opFlags & OpFlag::OP_FLAG_IS_BRANCH)) {
        branchStep = 2; // BRANCH_STEP_BRANCH
        inUncondJump = execOp->op == Op::J() || execOp->op == Op::JAL() ||
                       execOp->op == Op::JR();
      }

      if (didJump && branchStep == 1) { // BRANCH_STEP_DELAY
        cycle += 1;
        lastLoadPosMask >>= 1;
        didJump = false;

        if (inUncondJump) {
          lastLoadPosMask = 0;
          std::fill(std::begin(regStallExpiry), std::end(regStallExpiry), 0);
          inUncondJump = false;
        }
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

    // Control-register pairing quirk (VCO/VCC/VCE): the issue logic treats
    // a VU instruction that merely *reads* a control register as if it
    // also wrote it. So "vmrg ; cfc2 $vcc" never pairs (while the swapped
    // order does), and "ctc2 $vco ; vadd" never pairs either. This is a
    // pairing-only fact — there is no data dependency behind it, so it is
    // deliberately not part of the logical dependency masks.
    if (canDualIssue) {
      uint64_t defA = (op->opFlags & OpFlag::OP_FLAG_IS_VECTOR)
                          ? (op->depsSourceMask[4] | op->depsTargetMask[4])
                          : op->depsTargetMask[4];
      uint64_t accB = opNext->depsSourceMask[4] | opNext->depsTargetMask[4];
      if (defA & accB & CTRL_REG_MASK4) canDualIssue = false;
    }

    execCount = canDualIssue ? 2 : 1;
    cycle += 1;
    lastLoadPosMask >>= 1;
  }
  return cycle;
}

// --- Path-aware function cost ----------------------------------------------
// The anneal objective is the cost of the *hot path* — the fall-through
// spine of the function — not a linear walk over every instruction. Cold
// code (@Unlikely blocks parked after the function tail, skipped arms) is
// costed separately with a low weight so it stays sane but never trades
// against a hot-path cycle.
//
// Hot walk rules:
//   - conditional branches fall through (not taken); a backward one marks
//     a loop range, whose instructions get a per-nesting-level multiplier
//   - forward unconditional jumps (j / beq $zero,$zero) to a label inside
//     the function are followed (the skipped arm becomes an alternative)
//   - backward unconditional jumps are loop back-edges: range marked, not
//     followed
//   - jr, and j to a label outside the function, terminate the walk (after
//     their delay slot)
//   - jal continues after the call
//
// Weights (integer-scaled so the annealer keeps exact comparisons):
//   hot = 64 (× 8 per loop level, max 3 levels), alternative arm before the
//   terminal = 16, cold code after the terminal = 1.

namespace {
constexpr int W_HOT = 64;
constexpr int W_ALT = 16;
constexpr int W_COLD = 1;
constexpr int LOOP_MUL = 8;
constexpr int LOOP_MAX_DEPTH = 3;

bool isUncondBranch(const AsmInst &inst) {
  if (inst.op == Op::J()) return true;
  if (inst.op == Op::BEQ() && inst.args.size() >= 3 &&
      inst.args[0] == "$zero" && inst.args[1] == "$zero")
    return true;
  return false;
}

const std::string *branchTargetLabel(const AsmInst &inst) {
  if (!inst.cold->labelEnd.empty()) return &inst.cold->labelEnd;
  if (inst.args.empty()) return nullptr;
  return &inst.args.back();
}
} // namespace

HotPath evalCollectHotPath(AsmFunc &func) {
  HotPath hp;
  static thread_local std::unordered_map<std::string, int> labelPos;
  labelPos.clear();
  std::vector<int> ops; // asm_ indices of OP entries
  int lastLabel = -1;
  for (int a = 0; a < (int)func.asm_.size(); ++a) {
    const AsmInst &inst = func.asm_[a];
    if (inst.type == AsmType::OP) {
      hp.asmIndex.push_back(a);
      hp.labelBefore.push_back(lastLabel);
      lastLabel = -1;
    } else if (inst.type == AsmType::LABEL) {
      labelPos[inst.cold->label] = (int)hp.asmIndex.size(); // next op
      lastLabel = a;
    }
  }
  const int n = (int)hp.asmIndex.size();
  hp.opCount = n;
  hp.visited.assign(n, 0);
  hp.terminal = n;
  if (n == 0) return hp;
  auto opAt = [&](int i) -> const AsmInst & { return func.asm_[hp.asmIndex[i]]; };

  std::vector<int> targetIdx(n, -1);
  for (int i = 0; i < n; ++i) {
    const AsmInst &inst = opAt(i);
    if (!(inst.opFlags & OpFlag::OP_FLAG_IS_BRANCH)) continue;
    if (inst.op == Op::JR() || inst.op == Op::JAL()) continue;
    const std::string *lbl = branchTargetLabel(inst);
    if (!lbl) continue;
    auto it = labelPos.find(*lbl);
    if (it != labelPos.end() && it->second < n) targetIdx[i] = it->second;
  }

  int pc = 0;
  auto visit = [&](int idx) { hp.visited[idx] = 1; hp.ops.push_back(idx); };
  while (pc < n && !hp.visited[pc]) {
    visit(pc);
    const AsmInst &inst = opAt(pc);
    if (!(inst.opFlags & OpFlag::OP_FLAG_IS_BRANCH)) { ++pc; continue; }
    auto takeDelaySlot = [&]() { if (pc + 1 < n && !hp.visited[pc + 1]) visit(pc + 1); };
    if (inst.op == Op::JR()) { takeDelaySlot(); hp.terminal = pc + 2; break; }
    if (inst.op == Op::JAL()) { takeDelaySlot(); pc += 2; continue; }
    int tgt = targetIdx[pc];
    if (isUncondBranch(inst)) {
      if (tgt < 0) {
        if (inst.op == Op::J()) { takeDelaySlot(); hp.terminal = pc + 2; break; }
        pc += 1; continue;
      }
      if (tgt <= pc) { hp.loops.push_back({tgt, pc}); takeDelaySlot(); pc += 2; continue; }
      takeDelaySlot(); pc = tgt; continue;
    }
    if (tgt >= 0 && tgt <= pc) hp.loops.push_back({tgt, pc});
    pc += 1;
  }
  return hp;
}

int evalFunctionCost(AsmFunc &func) {
  static thread_local std::vector<AsmInst *> seq;
  HotPath hp = evalCollectHotPath(func);
  const int n = hp.opCount;
  if (n == 0) { func.hotCycles = 0; return 0; }

  // Conditional branches on the hot path are not taken (the walk fell
  // through them), so they carry no taken-branch bubble; only followed
  // jumps (j / beq $zero,$zero / jr) do. Mask the likely-flag for the
  // duration of the evaluation.
  std::vector<AsmInst *> masked;
  seq.clear();
  for (int idx : hp.ops) {
    AsmInst *op = &func.asm_[hp.asmIndex[idx]];
    if ((op->opFlags & OpFlag::OP_FLAG_IS_BRANCH) && !isUncondBranch(*op) &&
        op->op != Op::JR() && op->op != Op::J() &&
        (op->opFlags & OpFlag::OP_FLAG_LIKELY_BRANCH)) {
      op->opFlags &= ~OpFlag::OP_FLAG_LIKELY_BRANCH;
      masked.push_back(op);
    }
    seq.push_back(op);
  }
  int hotCycles = evalSequence(seq);
  for (AsmInst *op : masked) op->opFlags |= OpFlag::OP_FLAG_LIKELY_BRANCH;

  int64_t scaled = 0;
  int prevCycle = 0;
  for (size_t s = 0; s < seq.size(); ++s) {
    int delta = seq[s]->debug.cycle - prevCycle;
    prevCycle = seq[s]->debug.cycle;
    if (delta <= 0) continue;
    int weight = W_HOT;
    if (!hp.loops.empty()) {
      int opIdx = hp.ops[s];
      int depth = 0;
      for (const auto &lr : hp.loops)
        if (opIdx >= lr.first && opIdx <= lr.second) ++depth;
      for (int d = 0; d < std::min(depth, LOOP_MAX_DEPTH); ++d) weight *= LOOP_MUL;
    }
    scaled += (int64_t)delta * weight;
  }

  // Cold / alternative regions: each maximal unvisited run is costed as its
  // own straight-line sequence from a clean state.
  int i = 0;
  while (i < n) {
    if (hp.visited[i]) { ++i; continue; }
    int start = i;
    seq.clear();
    while (i < n && !hp.visited[i]) seq.push_back(&func.asm_[hp.asmIndex[i++]]);
    int cycles = evalSequence(seq);
    scaled += (int64_t)cycles * (start >= hp.terminal ? W_COLD : W_ALT);
  }

  func.hotCycles = hotCycles;
  if (scaled > INT32_MAX) scaled = INT32_MAX;
  return (int)scaled;
}

int evalFunctionCostLinear(AsmFunc &func) {
  static thread_local std::vector<AsmInst *> ops;
  ops.clear();
  for (auto &inst : func.asm_)
    if (inst.type == AsmType::OP) ops.push_back(&inst);
  return evalSequence(ops);
}

} // namespace rspl

#include "compact_sched.h"
#include "asm_scan_deps.h"

#include <algorithm>
#include <string>
#include <unordered_map>

namespace rspl {

namespace {

using RegMask = std::array<uint64_t, 5>;

bool maskAnd(const RegMask &a, const RegMask &b) {
  for (int i = 0; i < 5; ++i) if (a[i] & b[i]) return true;
  return false;
}
bool maskGetBit(const RegMask &m, int idx) { return (m[idx / 64] >> (idx % 64)) & 1ULL; }
void maskSetBit(RegMask &m, int idx) { m[idx / 64] |= (1ULL << (idx % 64)); }

inline bool isBranch(const CompactOp &o) { return o.flags & OpFlag::OP_FLAG_IS_BRANCH; }
inline bool isVector(const CompactOp &o) { return o.flags & OpFlag::OP_FLAG_IS_VECTOR; }

// port of checkAsmBackwardDep: does `later` depend on `earlier`?
bool backwardDep(const CompactOp &later, const CompactOp &earlier) {
  if (!later.isOp || !earlier.isOp) return true;
  if (maskAnd(earlier.tgt, later.src)) return true;
  if (maskAnd(earlier.src, later.tgt)) return true;
  if (later.barrier & earlier.barrier) return true;
  return false;
}

const std::unordered_map<Opcode, int> &vecScale() {
  static const std::unordered_map<Opcode, int> m = [] {
    std::unordered_map<Opcode, int> r;
    auto add = [&](const char *op, int s) { r[getOpcode(op)] = s; };
    add("lbv", 1);  add("sbv", 1);
    add("lsv", 2);  add("ssv", 2);
    add("llv", 4);  add("slv", 4);
    add("ldv", 8);  add("sdv", 8);
    add("lpv", 8);  add("spv", 8);
    add("luv", 8);  add("suv", 8);
    add("lqv", 16); add("sqv", 16);
    return r;
  }();
  return m;
}

const std::string *branchTargetLabel(const AsmInst &inst) {
  if (!inst.cold->labelEnd.empty()) return &inst.cold->labelEnd;
  if (inst.args.empty()) return nullptr;
  return &inst.args.back();
}

CompactOp fromInst(const AsmInst &inst) {
  CompactOp o;
  o.isOp = inst.type == AsmType::OP;
  o.isNop = o.isOp && (inst.opFlags & OpFlag::OP_FLAG_IS_NOP);
  o.flags = o.isOp ? inst.opFlags : 0;
  if (o.isOp) {
    o.isJ = inst.op == Op::J();
    o.isJal = inst.op == Op::JAL();
    o.isJr = inst.op == Op::JR();
    o.isUncond = o.isJ || (inst.op == Op::BEQ() && inst.args.size() >= 3 &&
                           inst.args[0] == "$zero" && inst.args[1] == "$zero");
  }
  o.stallLatency = inst.stallLatency;
  o.barrier = inst.barrierMask;
  o.src = inst.depsSourceMask; o.tgt = inst.depsTargetMask; o.arg = inst.depsArgMask;
  o.stSrc0 = inst.depsStallSourceMask0; o.stSrc1 = inst.depsStallSourceMask1;
  o.stTgt0 = inst.depsStallTargetMask0; o.stTgt1 = inst.depsStallTargetMask1;
  o.tgtIdx = inst.depsTargetIdx;
  o.stSrc = inst.depsStallSourceIdx; o.stTgt = inst.depsStallTargetIdx;
  o.rebaseKind = inst.rebaseKind; o.rebaseBase = inst.rebaseBase; o.rebaseValue = inst.rebaseValue;
  if (o.isOp) { auto it = vecScale().find(inst.op); if (it != vecScale().end()) o.rebaseScale = it->second; }
  return o;
}

} // namespace

// --- build -----------------------------------------------------------------

CompactFunc compactBuild(const AsmFunc &func) {
  CompactFunc cf;
  AsmFunc tmp; tmp.asm_ = func.asm_;
  asmInitDeps(tmp);
  std::unordered_map<std::string, int> labelId;
  for (const auto &inst : tmp.asm_) {
    cf.orig.push_back(inst);
    cf.ops.push_back(fromInst(inst));
    if (inst.type == AsmType::LABEL) labelId[inst.cold->label] = (int)cf.ops.size() - 1;
  }
  for (size_t i = 0; i < tmp.asm_.size(); ++i) {
    const AsmInst &inst = tmp.asm_[i];
    if (inst.type != AsmType::OP || !(inst.opFlags & OpFlag::OP_FLAG_IS_BRANCH)) continue;
    if (const std::string *lbl = branchTargetLabel(inst)) {
      auto it = labelId.find(*lbl);
      if (it != labelId.end()) cf.ops[i].targetLabel = it->second;
    }
  }
  AsmInst nop = asmNOP();
  asmInitDep(nop);
  cf.orig.push_back(nop);
  cf.ops.push_back(fromInst(nop));
  cf.nopId = (int)cf.ops.size() - 1;
  return cf;
}

CompactState compactInitialState(const CompactFunc &cf) {
  CompactState st;
  int n = (int)cf.ops.size();
  st.rebaseValue.resize(n); st.stall.assign(n, 0); st.paired.assign(n, 0); st.reorderCount.assign(n, 0);
  for (int i = 0; i < n; ++i) st.rebaseValue[i] = cf.ops[i].rebaseValue;
  for (int i = 0; i < n - 1; ++i) st.seq.push_back(cf.ops[i].isNop ? cf.nopId : i);
  return st;
}

// --- reorder range (port of asmGetReorderIndices) ----------------------------

std::vector<int> compactReorderIndices(const CompactFunc &cf, const CompactState &st, int i) {
  const auto &seq = st.seq;
  const CompactOp &A = cf.ops[seq[i]];
  if (A.flags & OpFlag::OP_FLAG_IS_IMMOVABLE) return {i};

  int lastWrite[REG_INDEX_SIZE] = {};
  RegMask lastWriteMask{}, lastReadMask{};
  const int size = (int)seq.size();
  int pos = size;
  bool isPastBranch = false;
  int f;
  for (f = i + 1; f < size; ++f) {
    const CompactOp &next = cf.ops[seq[f]];
    const CompactOp *prevPrev = (f >= 2) ? &cf.ops[seq[f - 2]] : nullptr;
    bool isFilledBranch = isBranch(next) && !((f + 1 < size) && cf.ops[seq[f + 1]].isNop);
    isPastBranch = prevPrev && isBranch(*prevPrev);
    if (isFilledBranch || isPastBranch || backwardDep(next, A)) { pos = f; break; }
    for (int reg : next.tgtIdx) { lastWrite[reg] = f; maskSetBit(lastWriteMask, reg); }
  }
  int fRead = isPastBranch ? f - 2 : f;
  for (; fRead < size; ++fRead) {
    const CompactOp &o = cf.ops[seq[fRead]];
    for (int w = 0; w < 5; ++w) lastReadMask[w] |= o.src[w];
    if (isBranch(o)) for (int w = 0; w < 5; ++w) lastReadMask[w] |= o.arg[w];
  }
  for (int reg : A.tgtIdx) {
    int lw = lastWrite[reg];
    if (lw && maskGetBit(lastReadMask, reg)) pos = std::min(lw, pos);
  }
  std::vector<int> res;
  for (int r = i; r <= pos - 1; ++r) res.push_back(r);
  RegMask writeCheck = A.tgt;
  for (int w = 0; w < 5; ++w) writeCheck[w] &= ~lastWriteMask[w];
  for (int b = i - 1; b >= 0; --b) {
    const CompactOp &prev = cf.ops[seq[b]];
    bool stop = (b >= 1 && isBranch(cf.ops[seq[b - 1]])) || backwardDep(A, prev) ||
                maskAnd(prev.tgt, writeCheck);
    if (stop) break;
    res.push_back(b);
  }
  return res;
}

// --- relocate (port of relocateElement) --------------------------------------

void compactRelocate(const CompactFunc &cf, CompactState &st, int from, int to) {
  auto &seq = st.seq;
  if (from == to) return;
  if (isBranch(cf.ops[seq[to]])) return;
  bool targetIsNOP = cf.ops[seq[to]].isNop;
  bool sourceInDelaySlot = (from >= 1) && isBranch(cf.ops[seq[from - 1]]);
  if (sourceInDelaySlot) {
    if (targetIsNOP) {
      // move the op out of the delay slot onto the NOP and leave a NOP in
      // the slot (the original relocateElement copied instead of moving here,
      // duplicating the instruction)
      seq[to] = seq[from];
      seq[from] = cf.nopId;
    } else {
      int inst = seq[from];
      seq[from] = cf.nopId;
      seq.insert(seq.begin() + to, inst);
    }
  } else {
    if (targetIsNOP) {
      seq[to] = seq[from];
      seq.erase(seq.begin() + from);
    } else {
      int inst = seq[from];
      seq.erase(seq.begin() + from);
      if (to > from) --to;
      seq.insert(seq.begin() + to, inst);
    }
  }
}

// --- rebase hop (port of asmTryRebaseCross) ----------------------------------

static bool rebaseApplyDelta(const CompactOp &op, int32_t &value, int delta) {
  if (op.rebaseKind != RebaseKind::MemOp) return false;
  int newOffset = value + delta;
  if (op.rebaseScale > 0) {
    int scale = op.rebaseScale;
    if (newOffset % scale != 0) return false;
    if (newOffset < -64 * scale || newOffset > 63 * scale) return false;
  } else {
    if (newOffset < -32768 || newOffset > 32767) return false;
  }
  value = newOffset;
  return true;
}

bool compactTryRebaseCross(const CompactFunc &cf, CompactState &st, int i, bool forward) {
  auto &seq = st.seq;
  const int sz = (int)seq.size();
  if (i < 0 || i >= sz) return false;
  const int mid = seq[i];
  const CompactOp &m = cf.ops[mid];
  if (!m.isOp || m.rebaseKind != RebaseKind::MemOp) return false;
  if (m.flags & (OpFlag::OP_FLAG_IS_IMMOVABLE | OpFlag::OP_FLAG_IS_NOP)) return false;

  if (forward) {
    for (int f = i + 1; f < sz; ++f) {
      const CompactOp &next = cf.ops[seq[f]];
      if (!next.isOp) return false;
      if (isBranch(next)) return false;
      if (maskAnd(next.tgt, m.tgt)) return false;
      if (backwardDep(next, m)) {
        if (next.rebaseKind != RebaseKind::Increment || next.rebaseBase != m.rebaseBase ||
            (next.barrier & m.barrier))
          return false;
        if (f + 1 >= sz) return false;
        if (!rebaseApplyDelta(m, st.rebaseValue[mid], -st.rebaseValue[seq[f]])) return false;
        bool inDelaySlot = (i >= 1) && isBranch(cf.ops[seq[i - 1]]);
        if (inDelaySlot) {
          seq[i] = cf.nopId;
          seq.insert(seq.begin() + (f + 1), mid);
        } else {
          seq.erase(seq.begin() + i);
          seq.insert(seq.begin() + f, mid);
        }
        return true;
      }
    }
    return false;
  }
  for (int b = i - 1; b >= 0; --b) {
    const CompactOp &prev = cf.ops[seq[b]];
    if (!prev.isOp) return false;
    if (isBranch(prev)) return false;
    if (b >= 1 && isBranch(cf.ops[seq[b - 1]])) return false;
    if (maskAnd(prev.tgt, m.tgt)) return false;
    if (backwardDep(m, prev)) {
      if (prev.rebaseKind != RebaseKind::Increment || prev.rebaseBase != m.rebaseBase ||
          (prev.barrier & m.barrier))
        return false;
      if (!rebaseApplyDelta(m, st.rebaseValue[mid], st.rebaseValue[seq[b]])) return false;
      seq.erase(seq.begin() + i);
      seq.insert(seq.begin() + b, mid);
      return true;
    }
  }
  return false;
}

// --- eval (port of evalSequence + evalFunctionCost) --------------------------

namespace {

constexpr uint64_t CTRL_REG_MASK4 =
    (1ULL << (288 - 256)) | (1ULL << (289 - 256)) | (1ULL << (294 - 256));
constexpr int W_HOT = 64, W_ALT = 16, W_COLD = 1, LOOP_MUL = 8, LOOP_MAX_DEPTH = 3;

struct SeqEntry { int id; bool likely; int cycle; };

// `ops`: item ids; writes cycle per entry, stall/paired feedback per id
int evalSeq(const CompactFunc &cf, CompactState &st, std::vector<SeqEntry> &ops) {
  if (ops.empty()) return 0;
  int regStallExpiry[64] = {};
  int cycle = 0, pc = 0, execCount = 0;
  uint64_t lastLoadPosMask = 0;
  int branchStep = 0;
  bool didJump = false, inUncondJump = false;
  const int n = (int)ops.size();
  while (pc < n) {
    bool resolved;
    do {
      resolved = true;
      for (int i = 0; i < execCount && pc + i < n; ++i) {
        const CompactOp &o = cf.ops[ops[pc + i].id];
        st.paired[ops[pc + i].id] = (execCount == 2);
        for (int src : o.stSrc) {
          if (regStallExpiry[src] > cycle) {
            int advance = regStallExpiry[src] - cycle;
            cycle += advance; lastLoadPosMask >>= advance; resolved = false;
          }
        }
        if ((lastLoadPosMask & 0b001) && (o.flags & OpFlag::OP_FLAG_IS_MEM_STALL_STORE)) {
          st.stall[ops[pc + i].id]++;
          cycle += 1; lastLoadPosMask >>= 1; resolved = false;
        }
      }
    } while (!resolved);
    for (int i = 0; i < execCount && pc + i < n; ++i) {
      SeqEntry &e = ops[pc + i];
      const CompactOp &o = cf.ops[e.id];
      if (o.flags & OpFlag::OP_FLAG_IS_MEM_STALL_LOAD) lastLoadPosMask |= 0b100;
      didJump |= e.likely;
      branchStep >>= 1;
      if (!branchStep && isBranch(o)) { branchStep = 2; inUncondJump = o.isJ || o.isJal || o.isJr; }
      if (didJump && branchStep == 1) {
        cycle += 1; lastLoadPosMask >>= 1; didJump = false;
        if (inUncondJump) { lastLoadPosMask = 0; std::fill(std::begin(regStallExpiry), std::end(regStallExpiry), 0); inUncondJump = false; }
      }
      e.cycle = cycle;
      for (int dst : o.stTgt) regStallExpiry[dst] = cycle + o.stallLatency;
    }
    pc += execCount;
    if (pc >= n) break;
    const CompactOp &op = cf.ops[ops[pc].id];
    const CompactOp *opNext = (pc + 1 < n) ? &cf.ops[ops[pc + 1].id] : nullptr;
    bool canDual = opNext && (isVector(op) != isVector(*opNext)) && !(branchStep == 2) && !isBranch(op) &&
                   (op.stTgt0 & opNext->stSrc0) == 0 && (op.stTgt1 & opNext->stSrc1) == 0 &&
                   (op.stTgt0 & opNext->stTgt0) == 0 && (op.stTgt1 & opNext->stTgt1) == 0;
    if (canDual && (opNext->flags & OpFlag::OP_FLAG_CTC2_CFC2)) {
      for (int w = 0; w < 5; ++w)
        if ((op.tgt[w] & opNext->src[w]) || (op.tgt[w] & opNext->tgt[w])) { canDual = false; break; }
    }
    if (canDual) {
      uint64_t defA = isVector(op) ? (op.src[4] | op.tgt[4]) : op.tgt[4];
      uint64_t accB = opNext->src[4] | opNext->tgt[4];
      if (defA & accB & CTRL_REG_MASK4) canDual = false;
    }
    execCount = canDual ? 2 : 1;
    cycle += 1; lastLoadPosMask >>= 1;
  }
  return cycle;
}

} // namespace

int compactEvalCost(const CompactFunc &cf, CompactState &st) {
  const auto &seq = st.seq;
  // op list (OP items only) + label positions, like evalCollectHotPath
  static thread_local std::vector<int> ops;      // seq index per op index
  static thread_local std::vector<int> labelPos; // label item id -> next op index
  ops.clear();
  labelPos.assign(cf.ops.size(), -1);
  for (int s = 0; s < (int)seq.size(); ++s) {
    const CompactOp &o = cf.ops[seq[s]];
    if (o.isOp) ops.push_back(s);
    else labelPos[seq[s]] = (int)ops.size();
  }
  const int n = (int)ops.size();
  if (n == 0) { st.hotCycles = 0; return 0; }
  auto opAt = [&](int i) -> const CompactOp & { return cf.ops[seq[ops[i]]]; };
  static thread_local std::vector<int> targetIdx;
  targetIdx.assign(n, -1);
  for (int i = 0; i < n; ++i) {
    const CompactOp &o = opAt(i);
    if (!isBranch(o) || o.isJr || o.isJal) continue;
    if (o.targetLabel >= 0 && labelPos[o.targetLabel] >= 0 && labelPos[o.targetLabel] < n)
      targetIdx[i] = labelPos[o.targetLabel];
  }
  static thread_local std::vector<uint8_t> visited;
  static thread_local std::vector<int> hot;
  static thread_local std::vector<std::pair<int, int>> loops;
  visited.assign(n, 0); hot.clear(); loops.clear();
  int pc = 0, terminal = n;
  auto visit = [&](int idx) { visited[idx] = 1; hot.push_back(idx); };
  while (pc < n && !visited[pc]) {
    visit(pc);
    const CompactOp &o = opAt(pc);
    if (!isBranch(o)) { ++pc; continue; }
    auto takeDelaySlot = [&]() { if (pc + 1 < n && !visited[pc + 1]) visit(pc + 1); };
    if (o.isJr) { takeDelaySlot(); terminal = pc + 2; break; }
    if (o.isJal) { takeDelaySlot(); pc += 2; continue; }
    int tgt = targetIdx[pc];
    if (o.isUncond) {
      if (tgt < 0) {
        if (o.isJ) { takeDelaySlot(); terminal = pc + 2; break; }
        pc += 1; continue;
      }
      if (tgt <= pc) { loops.push_back({tgt, pc}); takeDelaySlot(); pc += 2; continue; }
      takeDelaySlot(); pc = tgt; continue;
    }
    if (tgt >= 0 && tgt <= pc) loops.push_back({tgt, pc});
    pc += 1;
  }

  static thread_local std::vector<SeqEntry> sq;
  sq.clear();
  for (int idx : hot) {
    const CompactOp &o = opAt(idx);
    bool likely = o.flags & OpFlag::OP_FLAG_LIKELY_BRANCH;
    if (isBranch(o) && !o.isUncond && !o.isJr && !o.isJ) likely = false; // not taken on the hot walk
    sq.push_back({seq[ops[idx]], likely, 0});
  }
  int hotCycles = evalSeq(cf, st, sq);
  int64_t scaled = 0;
  int prevCycle = 0;
  for (size_t s = 0; s < sq.size(); ++s) {
    int delta = sq[s].cycle - prevCycle;
    prevCycle = sq[s].cycle;
    if (delta <= 0) continue;
    int weight = W_HOT;
    if (!loops.empty()) {
      int opIdx = hot[s], depth = 0;
      for (const auto &lr : loops) if (opIdx >= lr.first && opIdx <= lr.second) ++depth;
      for (int d = 0; d < std::min(depth, LOOP_MAX_DEPTH); ++d) weight *= LOOP_MUL;
    }
    scaled += (int64_t)delta * weight;
  }
  int i = 0;
  while (i < n) {
    if (visited[i]) { ++i; continue; }
    int start = i;
    sq.clear();
    while (i < n && !visited[i]) {
      const CompactOp &o = opAt(i);
      sq.push_back({seq[ops[i]], (bool)(o.flags & OpFlag::OP_FLAG_LIKELY_BRANCH), 0});
      ++i;
    }
    int cycles = evalSeq(cf, st, sq);
    scaled += (int64_t)cycles * (start >= terminal ? W_COLD : W_ALT);
  }
  st.hotCycles = hotCycles;
  return (int)scaled;
}

// --- apply -----------------------------------------------------------------

void compactApply(const CompactFunc &cf, const CompactState &st, AsmFunc &func) {
  std::vector<AsmInst> out;
  out.reserve(st.seq.size());
  for (int id : st.seq) {
    if (id == cf.nopId) { out.push_back(asmNOP()); continue; }
    AsmInst inst = cf.orig[id];
    const CompactOp &o = cf.ops[id];
    if (o.rebaseKind == RebaseKind::MemOp && st.rebaseValue[id] != o.rebaseValue) {
      int v = st.rebaseValue[id];
      if (o.rebaseScale > 0) inst.args[2] = std::to_string(v);
      else { auto par = inst.args[1].find('('); inst.args[1] = std::to_string(v) + inst.args[1].substr(par); }
    }
    inst.debug.reorderCount = st.reorderCount[id];
    inst.debug.paired = st.paired[id];
    inst.debug.stall = st.stall[id];
    out.push_back(std::move(inst));
  }
  func.asm_ = std::move(out);
  asmInitDeps(func);
}

} // namespace rspl

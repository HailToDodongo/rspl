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

static void buildPlan(CompactFunc &cf);

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
  buildPlan(cf);
  return cf;
}

CompactState compactInitialState(const CompactFunc &cf) {
  CompactState st;
  int n = (int)cf.ops.size();
  st.rebaseValue.resize(n); st.stall.assign(n, 0); st.paired.assign(n, 0);
  for (int i = 0; i < n; ++i) st.rebaseValue[i] = cf.ops[i].rebaseValue;
  // every original item keeps its own id (NOPs included, so they keep their
  // debug info on apply); cf.nopId is only used for NOPs the search inserts
  for (int i = 0; i < n - 1; ++i) st.seq.push_back(i);
  return st;
}

// --- reorder range (port of asmGetReorderIndices) ----------------------------

void compactReorderIndices(const CompactFunc &cf, const CompactState &st, int i,
                           std::vector<int> &res) {
  res.clear();
  const auto &seq = st.seq;
  const CompactOp &A = cf.ops[seq[i]];
  if (A.flags & OpFlag::OP_FLAG_IS_IMMOVABLE) { res.push_back(i); return; }

  // Generation-stamped last-write table: a slot counts as set only when its
  // stamp matches the current call, so there is no per-call clear of the
  // 295-entry table (the forward scan usually stops after a few items).
  static thread_local int lastWritePos[REG_INDEX_SIZE];
  static thread_local uint32_t lastWriteGen[REG_INDEX_SIZE];
  static thread_local uint32_t gen = 0;
  if (++gen == 0) { std::fill(std::begin(lastWriteGen), std::end(lastWriteGen), 0u); gen = 1; }
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
    for (int reg : next.tgtIdx) {
      lastWritePos[reg] = f; lastWriteGen[reg] = gen; maskSetBit(lastWriteMask, reg);
    }
  }
  // The reads-below scan only matters when an item inside the forward range
  // writes a register A also writes (a WAW pair whose later reader must not
  // see A instead). Without such a pair the scan cannot shrink `pos`, so
  // skip the walk to the end of the function entirely.
  if (maskAnd(A.tgt, lastWriteMask)) {
    int fRead = isPastBranch ? f - 2 : f;
    for (; fRead < size; ++fRead) {
      const CompactOp &o = cf.ops[seq[fRead]];
      for (int w = 0; w < 5; ++w) lastReadMask[w] |= o.src[w];
      if (isBranch(o)) for (int w = 0; w < 5; ++w) lastReadMask[w] |= o.arg[w];
    }
    for (int reg : A.tgtIdx) {
      if (lastWriteGen[reg] != gen) continue;
      if (maskGetBit(lastReadMask, reg)) pos = std::min(lastWritePos[reg], pos);
    }
  }
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
}

std::vector<int> compactReorderIndices(const CompactFunc &cf, const CompactState &st, int i) {
  std::vector<int> res;
  compactReorderIndices(cf, st, i, res);
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

struct SeqEntry { int id; int pos; bool likely; int cycle; int weight; };

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

bool likelyHotOf(const CompactOp &o) {
  bool likely = o.flags & OpFlag::OP_FLAG_LIKELY_BRANCH;
  if (isBranch(o) && !o.isUncond && !o.isJr && !o.isJ) likely = false; // not taken on the hot walk
  return likely;
}

// Unit segmentation of an order: [start, end) seq-index ranges holding OP
// items only. Per label: the body before it ends. Per branch: the body
// before it, the branch itself, and its delay-slot item (one item, or empty
// at the very end) are three units. The item order inside a unit is the
// only thing that varies between variants.
void segmentUnits(const CompactFunc &cf, const std::vector<int> &seq,
                  std::vector<int> &us, std::vector<int> &ue) {
  us.clear(); ue.clear();
  const int n = (int)seq.size();
  int s = 0;
  for (int k = 0; k < n; ++k) {
    const CompactOp &o = cf.ops[seq[k]];
    if (!o.isOp) { us.push_back(s); ue.push_back(k); s = k + 1; continue; }
    if (isBranch(o)) {
      us.push_back(s); ue.push_back(k);
      us.push_back(k); ue.push_back(k + 1);
      int slotEnd = (k + 1 < n && cf.ops[seq[k + 1]].isOp) ? k + 2 : k + 1;
      us.push_back(k + 1); ue.push_back(slotEnd);
      s = slotEnd; k = slotEnd - 1;
    }
  }
  us.push_back(s); ue.push_back(n);
}

// Op-level hot walk (port of evalCollectHotPath) over an order.
struct Walk {
  std::vector<int> ops;      // op index -> seq index
  std::vector<uint8_t> visited;
  std::vector<int> hot;      // op indices in visit order
  std::vector<std::pair<int, int>> loops;
  int terminal = 0;
};

void walkHot(const CompactFunc &cf, const std::vector<int> &seq, Walk &w) {
  static thread_local std::vector<int> labelPos, targetIdx;
  w.ops.clear(); w.hot.clear(); w.loops.clear();
  labelPos.assign(cf.ops.size(), -1);
  for (int s = 0; s < (int)seq.size(); ++s) {
    const CompactOp &o = cf.ops[seq[s]];
    if (o.isOp) w.ops.push_back(s);
    else labelPos[seq[s]] = (int)w.ops.size();
  }
  const int n = (int)w.ops.size();
  w.visited.assign(n, 0);
  w.terminal = n;
  if (n == 0) return;
  auto opAt = [&](int i) -> const CompactOp & { return cf.ops[seq[w.ops[i]]]; };
  targetIdx.assign(n, -1);
  for (int i = 0; i < n; ++i) {
    const CompactOp &o = opAt(i);
    if (!isBranch(o) || o.isJr || o.isJal) continue;
    if (o.targetLabel >= 0 && labelPos[o.targetLabel] >= 0 && labelPos[o.targetLabel] < n)
      targetIdx[i] = labelPos[o.targetLabel];
  }
  int pc = 0;
  auto visit = [&](int idx) { w.visited[idx] = 1; w.hot.push_back(idx); };
  while (pc < n && !w.visited[pc]) {
    visit(pc);
    const CompactOp &o = opAt(pc);
    if (!isBranch(o)) { ++pc; continue; }
    auto takeDelaySlot = [&]() { if (pc + 1 < n && !w.visited[pc + 1]) visit(pc + 1); };
    if (o.isJr) { takeDelaySlot(); w.terminal = pc + 2; break; }
    if (o.isJal) { takeDelaySlot(); pc += 2; continue; }
    int tgt = targetIdx[pc];
    if (o.isUncond) {
      if (tgt < 0) {
        if (o.isJ) { takeDelaySlot(); w.terminal = pc + 2; break; }
        pc += 1; continue;
      }
      if (tgt <= pc) { w.loops.push_back({tgt, pc}); takeDelaySlot(); pc += 2; continue; }
      takeDelaySlot(); pc = tgt; continue;
    }
    if (tgt >= 0 && tgt <= pc) w.loops.push_back({tgt, pc});
    pc += 1;
  }
}

int loopWeight(const Walk &w, int opIdx) {
  int weight = W_HOT;
  if (w.loops.empty()) return weight;
  int depth = 0;
  for (const auto &lr : w.loops) if (opIdx >= lr.first && opIdx <= lr.second) ++depth;
  for (int d = 0; d < std::min(depth, LOOP_MAX_DEPTH); ++d) weight *= LOOP_MUL;
  return weight;
}

void storeCycles(CompactState &st, const std::vector<SeqEntry> &sq) {
  st.cycle.resize(st.seq.size());
  for (const SeqEntry &e : sq) st.cycle[e.pos] = e.cycle;
}

int64_t scaledOf(const std::vector<SeqEntry> &sq) {
  int64_t scaled = 0;
  int prevCycle = 0;
  for (const SeqEntry &e : sq) {
    int delta = e.cycle - prevCycle;
    prevCycle = e.cycle;
    if (delta > 0) scaled += (int64_t)delta * e.weight;
  }
  return scaled;
}

} // namespace

// Full hot-path walk on every call; the plan-based compactEvalCost below is
// the fast path, this is what it falls back to without a valid plan.
static int compactEvalCostWalk(const CompactFunc &cf, CompactState &st) {
  const auto &seq = st.seq;
  static thread_local Walk w;
  walkHot(cf, seq, w);
  const int n = (int)w.ops.size();
  if (n == 0) { st.hotCycles = 0; return 0; }
  auto idAt = [&](int i) { return seq[w.ops[i]]; };

  static thread_local std::vector<SeqEntry> sq;
  sq.clear();
  for (int idx : w.hot)
    sq.push_back({idAt(idx), w.ops[idx], likelyHotOf(cf.ops[idAt(idx)]), 0, loopWeight(w, idx)});
  int hotCycles = evalSeq(cf, st, sq);
  int64_t scaled = scaledOf(sq);
  storeCycles(st, sq);

  int i = 0;
  while (i < n) {
    if (w.visited[i]) { ++i; continue; }
    int start = i;
    sq.clear();
    while (i < n && !w.visited[i]) {
      sq.push_back({idAt(i), w.ops[i], (bool)cf.likelyCold[idAt(i)], 0, 0});
      ++i;
    }
    int cycles = evalSeq(cf, st, sq);
    storeCycles(st, sq);
    scaled += (int64_t)cycles * (start >= w.terminal ? W_COLD : W_ALT);
  }
  st.hotCycles = hotCycles;
  return (int)scaled;
}

// Derive the unit-level plan from the initial order. Every op of a unit
// must agree on visited-status and weight (which the segmentation argument
// guarantees); if anything disagrees the plan is marked invalid and eval
// keeps walking per call.
static void buildPlan(CompactFunc &cf) {
  CompactPlan plan;
  cf.likelyHot.assign(cf.ops.size(), 0);
  cf.likelyCold.assign(cf.ops.size(), 0);
  for (size_t id = 0; id < cf.ops.size(); ++id) {
    cf.likelyCold[id] = (cf.ops[id].flags & OpFlag::OP_FLAG_LIKELY_BRANCH) ? 1 : 0;
    cf.likelyHot[id] = likelyHotOf(cf.ops[id]) ? 1 : 0;
  }

  CompactState st = compactInitialState(cf);
  Walk w;
  walkHot(cf, st.seq, w);
  std::vector<int> us, ue;
  segmentUnits(cf, st.seq, us, ue);
  const int unitCount = (int)us.size();
  const int n = (int)w.ops.size();
  plan.unitCount = unitCount;

  std::vector<int> unitOf(st.seq.size(), -1);
  for (int u = 0; u < unitCount; ++u)
    for (int k = us[u]; k < ue[u]; ++k) unitOf[k] = u;

  // per unit: visited status / weight, from its ops
  std::vector<int> unitVisited(unitCount, -1), unitWeight(unitCount, -1);
  bool ok = true;
  for (int i = 0; i < n; ++i) {
    int u = unitOf[w.ops[i]];
    int v = w.visited[i], wt = loopWeight(w, i);
    if (unitVisited[u] < 0) { unitVisited[u] = v; unitWeight[u] = wt; }
    else if (unitVisited[u] != v || unitWeight[u] != wt) ok = false;
  }
  // An empty body unit directly before a branch can gain the branch's
  // delay-slot op later; it behaves like the branch unit.
  for (int u = 0; u + 1 < unitCount; ++u) {
    if (unitVisited[u] >= 0) continue;
    if (us[u + 1] < (int)st.seq.size() && isBranch(cf.ops[st.seq[us[u + 1]]]) &&
        unitVisited[u + 1] >= 0) {
      unitVisited[u] = unitVisited[u + 1];
      unitWeight[u] = unitWeight[u + 1];
    }
  }
  auto isEmptyBodyBeforeBranch = [&](int u) {
    return us[u] == ue[u] && u + 1 < unitCount && us[u + 1] < (int)st.seq.size() &&
           isBranch(cf.ops[st.seq[us[u + 1]]]);
  };

  // hot units in visit order (an empty body goes right before its branch)
  std::vector<uint8_t> emitted(unitCount, 0);
  auto pushHot = [&](int u) {
    if (emitted[u]) { ok = false; return; }
    emitted[u] = 1;
    plan.hotUnits.push_back(u);
    plan.hotWeights.push_back(unitWeight[u]);
  };
  for (int idx : w.hot) {
    int u = unitOf[w.ops[idx]];
    if (!plan.hotUnits.empty() && plan.hotUnits.back() == u) continue;
    if (u >= 1 && isEmptyBodyBeforeBranch(u - 1) && !emitted[u - 1] && unitVisited[u - 1] == 1)
      pushHot(u - 1);
    pushHot(u);
  }
  // unvisited runs
  int i = 0;
  while (i < n) {
    if (w.visited[i]) { ++i; continue; }
    CompactPlan::Run run;
    run.weight = (i >= w.terminal) ? W_COLD : W_ALT;
    while (i < n && !w.visited[i]) {
      int u = unitOf[w.ops[i]];
      if (run.units.empty() || run.units.back() != u) {
        if (u >= 1 && isEmptyBodyBeforeBranch(u - 1) && unitVisited[u - 1] == 0 &&
            (run.units.empty() || run.units.back() != u - 1))
          run.units.push_back(u - 1);
        run.units.push_back(u);
      }
      ++i;
    }
    plan.runs.push_back(std::move(run));
  }
  plan.valid = ok;
  cf.plan = std::move(plan);
}

int compactEvalCost(const CompactFunc &cf, CompactState &st) {
  if (!cf.plan.valid) return compactEvalCostWalk(cf, st);
  const auto &seq = st.seq;
  static thread_local std::vector<int> us, ue;
  segmentUnits(cf, seq, us, ue);
  if ((int)us.size() != cf.plan.unitCount) return compactEvalCostWalk(cf, st);

  static thread_local std::vector<SeqEntry> sq;
  sq.clear();
  for (size_t h = 0; h < cf.plan.hotUnits.size(); ++h) {
    int u = cf.plan.hotUnits[h], wt = cf.plan.hotWeights[h];
    for (int k = us[u]; k < ue[u]; ++k) sq.push_back({seq[k], k, (bool)cf.likelyHot[seq[k]], 0, wt});
  }
  int hotCycles = evalSeq(cf, st, sq);
  int64_t scaled = scaledOf(sq);
  storeCycles(st, sq);

  for (const auto &run : cf.plan.runs) {
    sq.clear();
    for (int u : run.units)
      for (int k = us[u]; k < ue[u]; ++k) sq.push_back({seq[k], k, (bool)cf.likelyCold[seq[k]], 0, 0});
    if (sq.empty()) continue;
    int cycles = evalSeq(cf, st, sq);
    storeCycles(st, sq);
    scaled += (int64_t)cycles * run.weight;
  }
  st.hotCycles = hotCycles;
  return (int)scaled;
}

int compactEvalCostLinear(const CompactFunc &cf, CompactState &st) {
  static thread_local std::vector<SeqEntry> sq;
  sq.clear();
  for (int k = 0; k < (int)st.seq.size(); ++k)
    if (cf.ops[st.seq[k]].isOp) sq.push_back({st.seq[k], k, (bool)cf.likelyCold[st.seq[k]], 0, 0});
  int cycles = evalSeq(cf, st, sq);
  storeCycles(st, sq);
  return cycles;
}

// --- apply -----------------------------------------------------------------

void compactApply(const CompactFunc &cf, const CompactState &st, AsmFunc &func) {
  std::vector<AsmInst> out;
  out.reserve(st.seq.size());
  for (int id : st.seq) {
    AsmInst inst = cf.orig[id];
    // a NOP the search inserted into a delay slot: annotate like its branch
    if (id == cf.nopId && !out.empty()) inst.debug = out.back().debug;
    const CompactOp &o = cf.ops[id];
    if (o.rebaseKind == RebaseKind::MemOp && st.rebaseValue[id] != o.rebaseValue) {
      int v = st.rebaseValue[id];
      if (o.rebaseScale > 0) inst.args[2] = std::to_string(v);
      else { auto par = inst.args[1].find('('); inst.args[1] = std::to_string(v) + inst.args[1].substr(par); }
    }
    inst.debug.paired = st.paired[id];
    inst.debug.stall = st.stall[id];
    out.push_back(std::move(inst));
  }
  func.asm_ = std::move(out);
  asmInitDeps(func);
}

// --- AsmInst-list entry points (declared in asm_scan_deps.h) -----------------

static CompactFunc buildFromList(const std::vector<AsmInst> &asmList) {
  AsmFunc f;
  f.asm_ = asmList;
  return compactBuild(f);
}

std::vector<int> asmGetReorderIndices(const std::vector<AsmInst> &asmList, int i) {
  CompactFunc cf = buildFromList(asmList);
  CompactState st = compactInitialState(cf);
  return compactReorderIndices(cf, st, i);
}

bool asmTryRebaseCross(std::vector<AsmInst> &asmList, int i, bool forward) {
  CompactFunc cf = buildFromList(asmList);
  CompactState st = compactInitialState(cf);
  if (!compactTryRebaseCross(cf, st, i, forward)) return false;
  AsmFunc f;
  compactApply(cf, st, f);
  asmList = std::move(f.asm_);
  return true;
}

} // namespace rspl

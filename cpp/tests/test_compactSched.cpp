#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "asm_writer.h"
#include "optimizer/asm_scan_deps.h"
#include "optimizer/compact_sched.h"
#include "optimizer/eval_cost.h"
#include "asm_text_util.h"

#include <algorithm>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace rspl;

// The compact scheduler is the one implementation of the move rules; these
// tests pin its relocation semantics against an independent reference
// (the annealer's former AsmInst-based relocateElement) on random moves.

namespace {

std::vector<AsmInst> parseWithLabels(const std::string &text) {
  std::vector<AsmInst> out;
  std::istringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    size_t a = line.find_first_not_of(" \t");
    if (a == std::string::npos) continue;
    std::string t = line.substr(a);
    if (t.back() == ':') { out.push_back(asmLabel(t.substr(0, t.size() - 1))); continue; }
    auto v = textToAsmLines(t);
    for (auto &i : v) out.push_back(std::move(i));
  }
  return out;
}

// reference relocate (the annealer's former relocateElement, with the
// delay-slot-onto-NOP case fixed to move instead of copy)
void relocateRef(std::vector<AsmInst> &arr, int from, int to) {
  if (from == to) return;
  if (arr[to].opFlags & OpFlag::OP_FLAG_IS_BRANCH) return;
  bool targetIsNOP = arr[to].opFlags & OpFlag::OP_FLAG_IS_NOP;
  bool sourceInDelaySlot = (from >= 1) && (arr[from - 1].opFlags & OpFlag::OP_FLAG_IS_BRANCH);
  if (sourceInDelaySlot) {
    if (targetIsNOP) {
      arr[to] = arr[from];
      arr[from] = asmNOP();
      asmInitDep(arr[from]);
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

std::vector<std::string> texts(const std::vector<AsmInst> &l) {
  std::vector<std::string> r;
  for (const auto &i : l) r.push_back(i.type == AsmType::LABEL ? i.cold->label + ":" : stringifyInstr(i));
  return r;
}

const char *SAMPLE_A = R"(
[0] lqv $v01, 0, 0, $t0
[0] lqv $v02, 0, 16, $t0
[0] addiu $t0, $t0, 32
[0] lw $t2, 0($t3)
[0] vadd $v03, $v01, $v02
[0] vmudn $v04, $v03, $v31.e1
[0] addiu $t2, $t2, 1
[0] vlt $v05, $v03, $v04
[0] cfc2 $t4, $vcc
[0] vmrg $v06, $v01, $v02
[0] sqv $v04, 0, 0, $t1
[0] addiu $t1, $t1, 16
[0] sw $t2, 0($t3)
[0] bne $t4, $zero, SKIP # unlikely
[0] nop
[0] vsub $v07, $v06, $v05
[0] sqv $v07, 0, 0, $t1
SKIP:
[0] lhu $t5, 6($t3)
[0] vmudh $v29, $v05, $v30.e7
[0] vmadn $v08, $v06, $v30.e5
[0] vsar $v09, COP2_ACC_MD
[0] ssv $v09, 0, 4, $t1
[0] bne $t5, $zero, LOOP # unlikely
[0] nop
LOOP:
[0] addiu $t5, $t5, -1
[0] sqv $v08, 0, 16, $t1
[0] jr $ra
[0] nop
[0] mfc0 $t6, COP0_DMA_BUSY
[0] bne $t6, $zero, LOOP # unlikely
[0] nop
[0] j SKIP
[0] nop
)";

const char *SAMPLE_B = R"(
[0] vmulf $v06, $v20, $v07.h0
[0] ori $at, $zero, %lo(COLOR_AMBIENT)
[0] vmacf $v06, $v19, $v07.h1
[0] ori $s3, $zero, %lo(LIGHT_DIR_COLOR)
[0] vmacf $v07, $v18, $v07.h2
[0] vmudn $v06, $v28, $v08.h0
[0] vmadh $v05, $v27, $v08.h0
[0] vmadn $v06, $v26, $v08.h1
[0] luv $v03, 0, 0, $at
[0] vmadh $v05, $v25, $v08.h1
[0] vmadn $v06, $v24, $v08.h2
[0] vmadh $v05, $v23, $v08.h2
[0] vmadn $v06, $v22, $v08.h3
[0] luv $v04, 0, 16, $s4
[0] vmadh $v05, $v21, $v08.h3
[0] lhu $t0, 2($s4)
[0] vsar $v06, COP2_ACC_MD
[0] addiu $s4, $s4, 32
[0] vsar $v05, COP2_ACC_HI
[0] sh $t0, 4($s5)
[0] mtc2 $t0, $v10.e0
[0] vadd $v11, $v10, $v05
[0] sqv $v11, 0, 0, $s5
[0] addiu $s5, $s5, 16
[0] beq $t0, $zero, END # unlikely
[0] lqv $v12, 0, 0, $s5
[0] vand $v12, $v12, $v11
END:
[0] jr $ra
[0] sqv $v12, 0, 0, $s5
)";

// Random legal moves: the compact relocation, materialized through
// compactApply, must produce the same instruction list as the reference
// relocation on a plain AsmInst list. Also checks the plan-based cost stays
// consistent with itself after every move (stable hot cycle count).
void runRandomMoves(const char *text, uint32_t seed, int steps) {
  AsmFunc f; f.asm_ = parseWithLabels(text);
  asmInitDeps(f);
  CompactFunc cf = compactBuild(f);
  CompactState st = compactInitialState(cf);
  REQUIRE(cf.plan.valid);
  REQUIRE(compactEvalCost(cf, st) > 0);

  std::mt19937 rng(seed);
  int movesDone = 0;
  for (int k = 0; k < steps; ++k) {
    int sz = (int)f.asm_.size();
    REQUIRE((int)st.seq.size() == sz);
    int i = (int)(rng() % sz);
    auto range = compactReorderIndices(cf, st, i);
    if (range.size() > 1) {
      int t = i;
      while (t == i) t = range[rng() % range.size()];
      relocateRef(f.asm_, i, t);
      compactRelocate(cf, st, i, t);
      ++movesDone;
    }
    std::string err;
    bool ok = compactVerifySeq(cf, st.seq, &err);
    if (!ok) { AsmFunc dbg; compactApply(cf, st, dbg); for (auto &x : texts(dbg.asm_)) UNSCOPED_INFO(x); }
    INFO("step " << k << ": " << err);
    REQUIRE(ok);
    AsmFunc g; compactApply(cf, st, g);
    REQUIRE(texts(g.asm_) == texts(f.asm_));
    // evaluating twice from the same order must agree
    int c1 = compactEvalCost(cf, st);
    int h1 = st.hotCycles;
    int c2 = compactEvalCost(cf, st);
    REQUIRE(c1 == c2);
    REQUIRE(h1 == st.hotCycles);
  }
  REQUIRE(movesDone > 0);
}

} // namespace

TEST_CASE("Compact - relocation matches reference on random moves (A)", "[compact]") {
  runRandomMoves(SAMPLE_A, 1, 400);
  runRandomMoves(SAMPLE_A, 7, 400);
}

TEST_CASE("Compact - relocation matches reference on random moves (B)", "[compact]") {
  runRandomMoves(SAMPLE_B, 3, 400);
}

TEST_CASE("Compact - rebase hop rewrites the offset", "[compact]") {
  AsmFunc f; f.asm_ = parseWithLabels(SAMPLE_A);
  asmInitDeps(f);
  CompactFunc cf = compactBuild(f);
  CompactState st = compactInitialState(cf);
  int hops = 0;
  for (int i = 0; i < (int)st.seq.size(); ++i) {
    for (bool fwd : {true, false}) {
      if (compactTryRebaseCross(cf, st, i, fwd)) ++hops;
    }
  }
  REQUIRE(hops > 0);
  AsmFunc g; compactApply(cf, st, g);
  // every hop keeps the effective address: a rewritten offset shows up as a
  // different immediate, and the op count is unchanged
  REQUIRE(g.asm_.size() == f.asm_.size());
  REQUIRE(texts(g.asm_) != texts(f.asm_));
}

// Moving an op out of a branch delay slot onto a NOP must move it (slot
// becomes NOP), not duplicate it.
TEST_CASE("Compact - relocate out of delay slot onto NOP moves, not copies", "[compact]") {
  AsmFunc f; f.asm_ = parseWithLabels(R"(
[0] addiu $t0, $t0, 1
[0] bne $t1, $zero, L # unlikely
[0] addiu $t2, $t2, 4
[0] nop
L:
[0] jr $ra
[0] nop
)");
  asmInitDeps(f);
  CompactFunc cf = compactBuild(f);
  CompactState st = compactInitialState(cf);
  compactRelocate(cf, st, 2, 3);
  AsmFunc g; compactApply(cf, st, g);
  auto t = texts(g.asm_);
  REQUIRE(t.size() == 7);
  REQUIRE(t[1] == "bne $t1, $zero, L");
  REQUIRE(t[2] == "nop");
  REQUIRE(t[3] == "addiu $t2, $t2, 4");
  int count = 0;
  for (auto &x : t) if (x == "addiu $t2, $t2, 4") ++count;
  REQUIRE(count == 1);
}

// --- $acc chains + verifier ------------------------------------------------

namespace {

std::vector<std::vector<std::string>> chainTexts(const CompactFunc &cf) {
  std::vector<std::vector<std::string>> r;
  for (const auto &c : cf.chains) {
    std::vector<std::string> m;
    for (int id : c.ids) m.push_back(stringifyInstr(cf.orig[id]));
    r.push_back(m);
  }
  return r;
}

const char *SAMPLE_CHAINS = R"(
[0] lw $t0, 0($t1)
[0] addiu $t2, $t0, 1
[0] vmudh $v01, $v02, $v03.e0
[0] vmadn $v04, $v05, $v03.e1
[0] vmudh $v06, $v07, $v03.e0
[0] vmadn $v08, $v09, $v03.e1
[0] sw $t2, 4($t1)
[0] jr $ra
[0] nop
)";

struct Built { AsmFunc f; CompactFunc cf; CompactState st; };
Built buildOf(const char *text) {
  Built b;
  b.f.asm_ = parseWithLabels(text);
  asmInitDeps(b.f);
  b.cf = compactBuild(b.f);
  b.st = compactInitialState(b.cf);
  return b;
}

bool verify(const Built &b, std::vector<int> seq, std::string &err) {
  bool ok = compactVerifySeq(b.cf, seq, &err);
  if (!ok) UNSCOPED_INFO(err);
  return ok;
}

} // namespace

TEST_CASE("Compact - chain table on the samples", "[compact][chains]") {
  Built a = buildOf(SAMPLE_A);
  auto ca = chainTexts(a.cf);
  REQUIRE(ca.size() == 6);
  REQUIRE(ca[0] == std::vector<std::string>{"vadd $v03, $v01, $v02"});
  REQUIRE(ca[5] == std::vector<std::string>{"vmudh $v29, $v05, $v30.e7",
                                            "vmadn $v08, $v06, $v30.e5",
                                            "vsar $v09, COP2_ACC_MD"});
  for (int id = 0; id < (int)a.cf.ops.size(); ++id) {
    const CompactOp &o = a.cf.ops[id];
    bool vu = o.isOp && (o.flags & OpFlag::OP_FLAG_IS_VECTOR) &&
              stringifyInstr(a.cf.orig[id]).rfind("cfc2", 0) != 0;
    // every $acc user has a chain, nothing else does
    REQUIRE((a.cf.chainOf[id] >= 0) == vu);
  }

  Built b = buildOf(SAMPLE_B);
  auto cb = chainTexts(b.cf);
  REQUIRE(cb.size() == 4);
  REQUIRE(cb[0].size() == 3);   // vmulf + 2x vmacf
  REQUIRE(cb[1].size() == 10);  // vmudn .. vmadh + 2x vsar
  REQUIRE(cb[1].front() == "vmudn $v06, $v28, $v08.h0");
  REQUIRE(cb[1].back() == "vsar $v05, COP2_ACC_HI");
  REQUIRE(cb[2] == std::vector<std::string>{"vadd $v11, $v10, $v05"});
  REQUIRE(cb[3] == std::vector<std::string>{"vand $v12, $v12, $v11"});
}

TEST_CASE("Compact - verifier accepts the initial order and legal moves", "[compact][chains]") {
  Built b = buildOf(SAMPLE_CHAINS);
  std::string err;
  REQUIRE(b.cf.chains.size() == 2);
  REQUIRE(verify(b, {0, 1, 2, 3, 4, 5, 6, 7, 8}, err));
  // the two chains are register-independent: swapping them whole is legal
  REQUIRE(verify(b, {0, 1, 4, 5, 2, 3, 6, 7, 8}, err));
  // the store may fill the delay slot (the slot stays in its segment)
  REQUIRE(verify(b, {0, 1, 2, 3, 4, 5, 7, 6}, err));
  // scalar work may sink below the chains
  REQUIRE(verify(b, {0, 2, 3, 4, 5, 1, 6, 7, 8}, err));
}

TEST_CASE("Compact - verifier rejects broken dependencies", "[compact][chains]") {
  Built b = buildOf(SAMPLE_CHAINS);
  std::string err;
  REQUIRE_FALSE(verify(b, {1, 0, 2, 3, 4, 5, 6, 7, 8}, err));
  REQUIRE(err.rfind("RAW", 0) == 0);
  REQUIRE_FALSE(verify(b, {0, 6, 1, 2, 3, 4, 5, 7, 8}, err));
  REQUIRE(err.rfind("RAW", 0) == 0);
  // lw after the jr: left its segment
  REQUIRE_FALSE(verify(b, {1, 2, 3, 4, 5, 6, 7, 8, 0}, err));
  REQUIRE(err.find("segment") != std::string::npos);
  // duplicate / missing
  REQUIRE_FALSE(verify(b, {0, 0, 2, 3, 4, 5, 6, 7, 8}, err));
  REQUIRE_FALSE(verify(b, {0, 2, 3, 4, 5, 6, 7, 8}, err));
}

TEST_CASE("Compact - verifier rejects broken chains", "[compact][chains]") {
  Built b = buildOf(SAMPLE_CHAINS);
  std::string err;
  // interleaved
  REQUIRE_FALSE(verify(b, {0, 1, 2, 4, 3, 5, 6, 7, 8}, err));
  REQUIRE(err.find("interrupted") != std::string::npos);
  // members swapped inside a chain
  REQUIRE_FALSE(verify(b, {0, 1, 3, 2, 4, 5, 6, 7, 8}, err));
  REQUIRE(err.find("out of order") != std::string::npos);
  // one member left behind
  REQUIRE_FALSE(verify(b, {0, 1, 2, 4, 5, 3, 6, 7, 8}, err));
  REQUIRE(err.find("interrupted") != std::string::npos);
}

TEST_CASE("Compact - verifier lets accumulator readers swap", "[compact][chains]") {
  Built b = buildOf(R"(
[0] vmudn $v01, $v02, $v03.e0
[0] vmadh $v04, $v05, $v03.e1
[0] vsar $v06, COP2_ACC_MD
[0] vsar $v07, COP2_ACC_HI
[0] jr $ra
[0] nop
)");
  std::string err;
  REQUIRE(b.cf.chains.size() == 1);
  REQUIRE(b.cf.chains[0].ids.size() == 4);
  // vsar only reads $acc: the two may swap ...
  REQUIRE(verify(b, {0, 1, 3, 2, 4, 5}, err));
  // ... but not move before the last writer
  REQUIRE_FALSE(verify(b, {0, 2, 1, 3, 4, 5}, err));
  REQUIRE(err.find("out of order") != std::string::npos);
}

TEST_CASE("Compact - verifier WAW rule follows observed reads", "[compact][chains]") {
  Built b = buildOf(R"(
[0] addiu $t3, $zero, 1
[0] addiu $t3, $zero, 2
[0] addiu $t4, $zero, 3
[0] addiu $t4, $zero, 4
[0] sw $t4, 0($t1)
[0] jr $ra
[0] nop
)");
  std::string err;
  // $t3 is never read: the two dead writes may swap
  REQUIRE(verify(b, {1, 0, 2, 3, 4, 5, 6}, err));
  // $t4 is read after the second write: order matters
  REQUIRE_FALSE(verify(b, {0, 1, 3, 2, 4, 5, 6}, err));
  REQUIRE(err.rfind("WAW", 0) == 0);
}

TEST_CASE("Compact - verifier allows the rebase hop", "[compact][chains]") {
  Built b = buildOf(R"(
[0] lw $t0, 0($s0)
[0] addiu $s0, $s0, 4
[0] lw $t1, 0($s0)
[0] jr $ra
[0] nop
)");
  std::string err;
  // the load may hop across its base increment (offset rewritten by the
  // hop itself); the verifier only checks the order
  REQUIRE(verify(b, {0, 2, 1, 3, 4}, err));
  REQUIRE(verify(b, {1, 0, 2, 3, 4}, err));
}

// --- chain moves -------------------------------------------------------------

namespace {

std::vector<int> rangeOf(const Built &b, int pos, CompactRange &r, bool chainMoves = true) {
  std::vector<int> out;
  r = CompactRange{};
  r.chainMoves = chainMoves;
  compactReorderIndices(b.cf, b.st, pos, out, &r);
  return out;
}

std::vector<std::string> seqTexts(const Built &b) {
  AsmFunc g; compactApply(b.cf, b.st, g);
  return texts(g.asm_);
}

bool has(const std::vector<int> &v, int x) {
  return std::find(v.begin(), v.end(), x) != v.end();
}

const char *SAMPLE_C = R"(
[0] lqv $v01, 0, 0, $t0
[0] vmudl $v29, $v09, $v09.h3
[0] vmadm $v29, $v08, $v09.h3
[0] lw $t2, 0($t3)
[0] vmadn $v09, $v09, $v08.h3
[0] vmadh $v08, $v08, $v08.h3
[0] addiu $t2, $t2, 1
[0] vmudl $v29, $v05, $v02.v
[0] vmadm $v29, $v04, $v02.v
[0] vmadn $v05, $v05, $v01.v
[0] vmadh $v04, $v04, $v01.v
[0] vand $v10, $v10, $v31.e1
[0] sw $t2, 0($t3)
[0] vmudn $v11, $v12, $v13.e2
[0] vmadh $v12, $v12, $v13.e3
[0] sqv $v04, 0, 0, $t1
[0] sqv $v08, 0, 16, $t1
[0] jr $ra
[0] nop
)";

// Random plain + chain moves, verified after every step.
void runRandomChainMoves(const char *text, uint32_t seed, int steps, bool expectChainMoves) {
  Built b = buildOf(text);
  REQUIRE(compactEvalCost(b.cf, b.st) > 0);
  std::mt19937 rng(seed);
  int chainMoves = 0, plainMoves = 0, rejected = 0;
  std::vector<int> range;
  for (int k = 0; k < steps; ++k) {
    int sz = (int)b.st.seq.size();
    int i = (int)(rng() % sz);
    CompactRange r; r.chainMoves = true;
    compactReorderIndices(b.cf, b.st, i, range, &r);
    if (range.size() > 1) {
      int t = i;
      while (t == i) t = range[rng() % range.size()];
      if (t < r.plainLo || t > r.plainHi) {
        std::vector<int> before = b.st.seq;
        if (compactRelocateChain(b.cf, b.st, i, t)) ++chainMoves;
        else { ++rejected; REQUIRE(b.st.seq == before); }
      } else {
        compactRelocate(b.cf, b.st, i, t);
        ++plainMoves;
      }
    }
    std::string err;
    bool ok = compactVerifySeq(b.cf, b.st.seq, &err);
    if (!ok) for (auto &x : seqTexts(b)) UNSCOPED_INFO(x);
    INFO("step " << k << ": " << err);
    REQUIRE(ok);
    int c1 = compactEvalCost(b.cf, b.st);
    int c2 = compactEvalCost(b.cf, b.st);
    REQUIRE(c1 == c2);
  }
  REQUIRE(plainMoves > 0);
  if (expectChainMoves) REQUIRE(chainMoves > 0);
}

} // namespace

TEST_CASE("Compact - chain move: tail forward past a foreign chain", "[compact][chains]") {
  Built b = buildOf(SAMPLE_CHAINS);
  CompactRange r;
  auto range = rangeOf(b, 3, r); // vmadn of chain A (tail)
  REQUIRE(r.plainLo == 3);
  REQUIRE(r.plainHi == 3);
  // before B's head (4) and after B's tail (6); never inside B (5), never
  // the jr (7) or its delay slot (8)
  REQUIRE(has(range, 4));
  REQUIRE(has(range, 6));
  REQUIRE_FALSE(has(range, 5));
  REQUIRE_FALSE(has(range, 7));
  REQUIRE_FALSE(has(range, 8));
  REQUIRE(compactRelocateChain(b.cf, b.st, 3, 6));
  REQUIRE(seqTexts(b) == std::vector<std::string>{
      "lw $t0, 0($t1)", "addiu $t2, $t0, 1",
      "vmudh $v06, $v07, $v03.e0", "vmadn $v08, $v09, $v03.e1",
      "vmudh $v01, $v02, $v03.e0", "vmadn $v04, $v05, $v03.e1",
      "sw $t2, 4($t1)", "jr $ra", "nop"});
  std::string err;
  REQUIRE(verify(b, b.st.seq, err));
}

TEST_CASE("Compact - chain move: head backward past a foreign chain", "[compact][chains]") {
  Built b = buildOf(SAMPLE_CHAINS);
  CompactRange r;
  auto range = rangeOf(b, 4, r); // vmudh of chain B (head)
  REQUIRE(r.plainLo == 4);
  REQUIRE(r.plainHi == 4);
  REQUIRE(has(range, 2));      // before A's head
  REQUIRE_FALSE(has(range, 3)); // inside A
  REQUIRE(has(range, 1));
  REQUIRE(has(range, 0));
  REQUIRE(compactRelocateChain(b.cf, b.st, 4, 2));
  REQUIRE(seqTexts(b) == std::vector<std::string>{
      "lw $t0, 0($t1)", "addiu $t2, $t0, 1",
      "vmudh $v06, $v07, $v03.e0", "vmadn $v08, $v09, $v03.e1",
      "vmudh $v01, $v02, $v03.e0", "vmadn $v04, $v05, $v03.e1",
      "sw $t2, 4($t1)", "jr $ra", "nop"});
  std::string err;
  REQUIRE(verify(b, b.st.seq, err));
}

TEST_CASE("Compact - chain move: middle members and inner ends keep the plain range", "[compact][chains]") {
  Built b = buildOf(R"(
[0] vmudh $v01, $v02, $v03.e0
[0] vmadn $v04, $v05, $v03.e1
[0] vmadh $v06, $v07, $v03.e2
[0] vmudh $v08, $v09, $v03.e0
[0] vmadn $v10, $v11, $v03.e1
[0] jr $ra
[0] nop
)");
  CompactRange r;
  for (int pos : {1, 0}) { // middle member; head going forward meets a sibling
    auto withChains = rangeOf(b, pos, r, true);
    auto plain = rangeOf(b, pos, r, false);
    REQUIRE(withChains == plain);
  }
  // the tail of the second chain has nothing foreign ahead: plain as well
  REQUIRE(rangeOf(b, 4, r, true) == rangeOf(b, 4, r, false));
}

TEST_CASE("Compact - chain move: chain straddling a branch gets no chain targets", "[compact][chains]") {
  Built b = buildOf(R"(
[0] vmudh $v01, $v02, $v03.e0
[0] bne $t0, $zero, L # unlikely
[0] nop
[0] vmadn $v04, $v05, $v03.e1
[0] vmudh $v06, $v07, $v03.e0
[0] vmadn $v08, $v09, $v03.e1
L:
[0] jr $ra
[0] nop
)");
  REQUIRE(b.cf.chains.size() == 2);
  REQUIRE_FALSE(b.cf.chains[0].movable);
  REQUIRE(b.cf.chains[1].movable);
  CompactRange r;
  REQUIRE(rangeOf(b, 3, r, true) == rangeOf(b, 3, r, false));
}

TEST_CASE("Compact - chain move: delay slot only for single-member chains", "[compact][chains]") {
  Built b = buildOf(R"(
[0] vmudh $v01, $v02, $v03.e0
[0] vmadn $v04, $v05, $v03.e1
[0] vand $v10, $v10, $v31.e1
[0] bne $t0, $zero, L # unlikely
[0] nop
L:
[0] jr $ra
[0] nop
)");
  CompactRange r;
  auto range = rangeOf(b, 1, r); // tail of the 2-member chain
  REQUIRE(has(range, 2));       // before the vand
  REQUIRE_FALSE(has(range, 3)); // the branch
  REQUIRE_FALSE(has(range, 4)); // its delay slot: followers cannot get behind it

  Built c = buildOf(R"(
[0] vand $v10, $v10, $v31.e1
[0] vmudh $v01, $v02, $v03.e0
[0] vmadn $v04, $v05, $v03.e1
[0] bne $t0, $zero, L # unlikely
[0] nop
L:
[0] jr $ra
[0] nop
)");
  range = rangeOf(c, 0, r); // the lone vand, past the 2-member chain
  REQUIRE(r.plainHi == 0);
  REQUIRE_FALSE(has(range, 2)); // inside the chain
  REQUIRE(has(range, 4));       // the delay slot
  REQUIRE(compactRelocateChain(c.cf, c.st, 0, 4));
  auto t = seqTexts(c);
  REQUIRE(t[2] == "bne $t0, $zero, L");
  REQUIRE(t[3] == "vand $v10, $v10, $v31.e1");
  std::string err;
  REQUIRE(verify(c, c.st.seq, err));
}

TEST_CASE("Compact - chain move: follower conflict rejects and restores", "[compact][chains]") {
  Built b = buildOf(R"(
[0] lw $t0, 0($t1)
[0] vmudh $v01, $v02, $v03.e0
[0] vmadn $v04, $v05, $v03.e1
[0] vmudh $v06, $v01, $v03.e0
[0] vmadn $v08, $v09, $v03.e1
[0] sw $t0, 4($t1)
[0] jr $ra
[0] nop
)");
  CompactRange r;
  auto range = rangeOf(b, 2, r); // tail of A; B's head reads $v01 written by A's head
  REQUIRE(has(range, 5));     // the leader alone could pass B
  std::vector<int> before = b.st.seq;
  REQUIRE_FALSE(compactRelocateChain(b.cf, b.st, 2, 5));
  REQUIRE(b.st.seq == before);
}

TEST_CASE("Compact - chain move: random plain + chain moves stay legal", "[compact][chains]") {
  runRandomChainMoves(SAMPLE_CHAINS, 11, 300, true);
  runRandomChainMoves(SAMPLE_C, 5, 1500, true);
  runRandomChainMoves(SAMPLE_C, 17, 1500, true);
  runRandomChainMoves(SAMPLE_A, 1, 600, false);
  runRandomChainMoves(SAMPLE_B, 3, 600, false);
}

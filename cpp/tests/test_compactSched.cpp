#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "asm_writer.h"
#include "optimizer/asm_scan_deps.h"
#include "optimizer/compact_sched.h"
#include "optimizer/eval_cost.h"
#include "asm_text_util.h"

#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace rspl;

// The compact scheduler must be an exact port of the AsmInst-based logic:
// same legal move ranges, same relocation/rebase results, same cost.

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

void runEquivalence(const char *text, uint32_t seed, int steps) {
  AsmFunc f; f.asm_ = parseWithLabels(text);
  asmInitDeps(f);
  CompactFunc cf = compactBuild(f);
  CompactState st = compactInitialState(cf);
  REQUIRE(compactEvalCost(cf, st) == evalFunctionCost(f));
  REQUIRE(st.hotCycles == f.hotCycles);

  std::mt19937 rng(seed);
  int movesDone = 0, hops = 0;
  for (int k = 0; k < steps; ++k) {
    int sz = (int)f.asm_.size();
    REQUIRE((int)st.seq.size() == sz);
    int i = (int)(rng() % sz);
    if (rng() % 5 == 0) {
      bool fwd = rng() & 1;
      bool r1 = asmTryRebaseCross(f.asm_, i, fwd);
      bool r2 = compactTryRebaseCross(cf, st, i, fwd);
      REQUIRE(r1 == r2);
      if (r1) ++hops;
    } else {
      auto a = asmGetReorderIndices(f.asm_, i);
      auto b = compactReorderIndices(cf, st, i);
      REQUIRE(a == b);
      if (a.size() > 1) {
        int t = i;
        while (t == i) t = a[rng() % a.size()];
        relocateRef(f.asm_, i, t);
        compactRelocate(cf, st, i, t);
        ++movesDone;
      }
    }
    AsmFunc g; compactApply(cf, st, g);
    REQUIRE(texts(g.asm_) == texts(f.asm_));
    int c1 = evalFunctionCost(f);
    int c2 = compactEvalCost(cf, st);
    REQUIRE(c1 == c2);
    REQUIRE(f.hotCycles == st.hotCycles);
  }
  REQUIRE(movesDone > 0);
  (void)hops;
}

} // namespace

TEST_CASE("Compact - matches reference on random moves (A)", "[compact]") {
  runEquivalence(SAMPLE_A, 1, 400);
  runEquivalence(SAMPLE_A, 7, 400);
}

TEST_CASE("Compact - matches reference on random moves (B)", "[compact]") {
  runEquivalence(SAMPLE_B, 3, 400);
}

TEST_CASE("Compact - rebase hop rewrites offsets like the reference", "[compact]") {
  AsmFunc f; f.asm_ = parseWithLabels(SAMPLE_A);
  asmInitDeps(f);
  CompactFunc cf = compactBuild(f);
  CompactState st = compactInitialState(cf);
  int hops = 0;
  for (int i = 0; i < (int)f.asm_.size(); ++i) {
    for (bool fwd : {true, false}) {
      bool r1 = asmTryRebaseCross(f.asm_, i, fwd);
      bool r2 = compactTryRebaseCross(cf, st, i, fwd);
      REQUIRE(r1 == r2);
      if (r1) ++hops;
      AsmFunc g; compactApply(cf, st, g);
      REQUIRE(texts(g.asm_) == texts(f.asm_));
    }
  }
  REQUIRE(hops > 0);
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

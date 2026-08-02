#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "optimizer/asm_scan_deps.h"

#include <string>
#include <vector>

// Offset-rebase hop: a mem-op may cross a pure self-increment of its base
// register if its immediate offset is rewritten to keep the effective
// address identical (asmTryRebaseCross).

using namespace rspl;

static std::vector<AsmInst> build(std::vector<AsmInst> list) {
  for (auto &a : list) asmInitDep(a);
  return list;
}

// Invariant: for every rebasable mem-op, offset + sum of qualifying
// increments of its base that appear BEFORE it must be unchanged by hops.
static std::vector<int> effectiveAddresses(const std::vector<AsmInst> &list) {
  std::vector<int> res;
  int incSums[32] = {};
  for (const auto &inst : list) {
    if (inst.rebaseKind == RebaseKind::MemOp) {
      res.push_back(inst.rebaseValue + incSums[inst.rebaseBase]);
    } else if (inst.rebaseKind == RebaseKind::Increment) {
      incSums[inst.rebaseBase] += inst.rebaseValue;
    }
  }
  return res;
}

TEST_CASE("RebaseHop - forward across increment (vector)", "[rebaseHop]") {
  auto list = build({
      asmOp("sqv", {"$v01", "0", "0", "$t0"}),
      asmOp("addiu", {"$t0", "$t0", "16"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
  });
  auto before = effectiveAddresses(list);

  REQUIRE(asmTryRebaseCross(list, 0, true) == true);
  REQUIRE(getOpcodeName(list[0].op) == "addiu");
  REQUIRE(getOpcodeName(list[1].op) == "sqv");
  REQUIRE(list[1].args[2] == "-16");
  REQUIRE(effectiveAddresses(list) == before);
}

TEST_CASE("RebaseHop - backward across increment (vector)", "[rebaseHop]") {
  auto list = build({
      asmOp("addiu", {"$t0", "$t0", "16"}),
      asmOp("sqv", {"$v01", "0", "-16", "$t0"}),
  });
  auto before = effectiveAddresses(list);

  REQUIRE(asmTryRebaseCross(list, 1, false) == true);
  REQUIRE(getOpcodeName(list[0].op) == "sqv");
  REQUIRE(list[0].args[2] == "0");
  REQUIRE(getOpcodeName(list[1].op) == "addiu");
  REQUIRE(effectiveAddresses(list) == before);
}

TEST_CASE("RebaseHop - scalar store, offset(base) rewrite", "[rebaseHop]") {
  auto list = build({
      asmOp("sw", {"$t1", "8($t0)"}),
      asmOp("addiu", {"$t0", "$t0", "4"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  REQUIRE(asmTryRebaseCross(list, 0, true) == true);
  REQUIRE(getOpcodeName(list[1].op) == "sw");
  REQUIRE(list[1].args[1] == "4($t0)");
}

TEST_CASE("RebaseHop - crosses independent instructions on the way",
          "[rebaseHop]") {
  // blocker is not adjacent; the ops in between are unrelated
  auto list = build({
      asmOp("sqv", {"$v01", "0", "0", "$t0"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("vxor", {"$v02", "$v00", "$v00.e0"}),
      asmOp("addiu", {"$t0", "$t0", "32"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  auto before = effectiveAddresses(list);
  REQUIRE(asmTryRebaseCross(list, 0, true) == true);
  REQUIRE(getOpcodeName(list[3].op) == "sqv");
  REQUIRE(list[3].args[2] == "-32");
  REQUIRE(effectiveAddresses(list) == before);
}

TEST_CASE("RebaseHop - rejects base used as store data", "[rebaseHop]") {
  auto list = build({
      asmOp("sw", {"$t0", "0($t0)"}),
      asmOp("addiu", {"$t0", "$t0", "4"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  REQUIRE(list[0].rebaseKind == RebaseKind::None);
  REQUIRE(asmTryRebaseCross(list, 0, true) == false);
}

TEST_CASE("RebaseHop - rejects load whose dest is the base", "[rebaseHop]") {
  auto list = build({
      asmOp("lw", {"$t0", "0($t0)"}),
      asmOp("addiu", {"$t0", "$t0", "4"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  REQUIRE(list[0].rebaseKind == RebaseKind::None);
  REQUIRE(asmTryRebaseCross(list, 0, true) == false);
}

TEST_CASE("RebaseHop - rejects %lo() offsets", "[rebaseHop]") {
  auto list = build({
      asmOp("sw", {"$t1", "%lo(SOME_LABEL)($t0)"}),
      asmOp("addiu", {"$t0", "$t0", "4"}),
  });
  REQUIRE(list[0].rebaseKind == RebaseKind::None);
  REQUIRE(asmTryRebaseCross(list, 0, true) == false);
}

TEST_CASE("RebaseHop - rejects non-self increment", "[rebaseHop]") {
  // addiu writes the base but computes it from another register
  auto list = build({
      asmOp("sqv", {"$v01", "0", "0", "$t0"}),
      asmOp("addiu", {"$t0", "$t1", "16"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  REQUIRE(list[1].rebaseKind == RebaseKind::None);
  REQUIRE(asmTryRebaseCross(list, 0, true) == false);
}

TEST_CASE("RebaseHop - rejects misaligned result (sqv)", "[rebaseHop]") {
  // crossing a +4 increment would need offset -4: not 16-byte aligned
  auto list = build({
      asmOp("sqv", {"$v01", "0", "0", "$t0"}),
      asmOp("addiu", {"$t0", "$t0", "4"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  REQUIRE(asmTryRebaseCross(list, 0, true) == false);
  REQUIRE(list[0].args[2] == "0"); // unchanged
  REQUIRE(getOpcodeName(list[0].op) == "sqv"); // not moved
}

TEST_CASE("RebaseHop - rejects out-of-range result (lbv)", "[rebaseHop]") {
  // lbv offset range is -64..63; moving above "+= 1" needs 63 + 1 = 64
  auto list = build({
      asmOp("addiu", {"$t0", "$t0", "1"}),
      asmOp("lbv", {"$v01", "0", "63", "$t0"}),
  });
  REQUIRE(asmTryRebaseCross(list, 1, false) == false);
  REQUIRE(list[1].args[2] == "63"); // unchanged, not moved

  // in-range delta works: 63 + (-1) = 62
  auto list2 = build({
      asmOp("addiu", {"$t0", "$t0", "-1"}),
      asmOp("lbv", {"$v01", "0", "63", "$t0"}),
  });
  REQUIRE(asmTryRebaseCross(list2, 1, false) == true);
  REQUIRE(getOpcodeName(list2[0].op) == "lbv");
  REQUIRE(list2[0].args[2] == "62");
}

TEST_CASE("RebaseHop - rejects crossing a branch", "[rebaseHop]") {
  auto list = build({
      asmOp("sqv", {"$v01", "0", "0", "$t0"}),
      asmOp("beq", {"$t1", "$zero", "SOME_LABEL"}),
      asmOp("nop", {}),
      asmOp("addiu", {"$t0", "$t0", "16"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  REQUIRE(asmTryRebaseCross(list, 0, true) == false);
}

TEST_CASE("RebaseHop - rejects when blocked by a real dependency",
          "[rebaseHop]") {
  // something reads the stored range's base before the increment — crossing
  // the reader is fine (read-read), but a writer of the base blocks
  auto list = build({
      asmOp("sqv", {"$v01", "0", "0", "$t0"}),
      asmOp("or", {"$t0", "$zero", "$t3"}), // overwrites base, not an addiu
      asmOp("addiu", {"$t0", "$t0", "16"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  REQUIRE(asmTryRebaseCross(list, 0, true) == false);
}

TEST_CASE("RebaseHop - WAW guard for loads", "[rebaseHop]") {
  // vxor writes $v01 (all lanes) — the load must not cross it even though
  // there is no read-after-write between them
  auto list = build({
      asmOp("ldv", {"$v01", "0", "0", "$t0"}),
      asmOp("vxor", {"$v01", "$v00", "$v00.e0"}),
      asmOp("addiu", {"$t0", "$t0", "8"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  REQUIRE(asmTryRebaseCross(list, 0, true) == false);
}

TEST_CASE("RebaseHop - load crossing works and keeps deps valid",
          "[rebaseHop]") {
  auto list = build({
      asmOp("lqv", {"$v05", "0", "16", "$s0"}),
      asmOp("addiu", {"$s0", "$s0", "32"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  });
  auto before = effectiveAddresses(list);
  REQUIRE(asmTryRebaseCross(list, 0, true) == true);
  REQUIRE(getOpcodeName(list[1].op) == "lqv");
  REQUIRE(list[1].args[2] == "-16");
  REQUIRE(effectiveAddresses(list) == before);
  // rebase info stays consistent for a future second hop
  REQUIRE(list[1].rebaseKind == RebaseKind::MemOp);
  REQUIRE(list[1].rebaseValue == -16);
}

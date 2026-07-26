#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "optimizer/asm_scan_deps.h"

#include <algorithm>
#include <string>
#include <vector>

// Mirrors src/tests/reorderPartialLoad.test.js

using namespace rspl;

static std::vector<AsmInst> build() {
  std::vector<AsmInst> list = {
      asmOp("vmadn", {"$v06", "$v14", "$v30.e2"}), // 0: dead full write to $v06
      asmOp("ldv", {"$v06", "0", "0", "$s0"}),     // 1: lanes 0-3
      asmOp("ldv", {"$v06", "8", "0", "$s1"}),     // 2: lanes 4-7
      asmOp("vmulf", {"$v04", "$v06", "$v06"}),    // 3: reads $v06
  };
  for (auto &a : list) asmInitDep(a);
  return list;
}

// The dead vmadn must not be movable between the two half-register loads:
// doing so would clobber the lanes the first ldv already wrote.
TEST_CASE("Reorder partial load", "[reorderPartialLoad]") {
  auto list = build();
  auto range = asmGetReorderIndices(list, 0);
  bool canLandBetweenLdvs =
      std::find(range.begin(), range.end(), 1) != range.end();
  REQUIRE(canLandBetweenLdvs == false);
}

TEST_CASE("Partial load target lanes - aligned offset",
          "[reorderPartialLoad]") {
  REQUIRE(getTargetRegs(asmOp("ldv", {"$v06", "0", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e0", "$v06.e1", "$v06.e2", "$v06.e3"});
  REQUIRE(getTargetRegs(asmOp("ldv", {"$v06", "8", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e4", "$v06.e5", "$v06.e6", "$v06.e7"});
  REQUIRE(getTargetRegs(asmOp("llv", {"$v06", "4", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e2", "$v06.e3"});
  REQUIRE(getTargetRegs(asmOp("lsv", {"$v06", "2", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e1"});
  REQUIRE(getTargetRegs(asmOp("lbv", {"$v06", "4", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e2"});
}

TEST_CASE("Partial load target lanes - odd offset", "[reorderPartialLoad]") {
  // an odd offset straddles a lane boundary, touching one extra lane
  REQUIRE(getTargetRegs(asmOp("lbv", {"$v06", "5", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e2"});
  REQUIRE(getTargetRegs(asmOp("lsv", {"$v06", "3", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e1", "$v06.e2"});
  REQUIRE(getTargetRegs(asmOp("llv", {"$v06", "1", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e0", "$v06.e1", "$v06.e2"});
  // loads stop at the end of the register, lanes must be clamped
  REQUIRE(getTargetRegs(asmOp("ldv", {"$v06", "13", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e6", "$v06.e7"});
  REQUIRE(getTargetRegs(asmOp("lsv", {"$v06", "15", "0", "$s0"})) ==
          std::vector<std::string>{"$v06.e7"});
}

// A ".v" source suffix must expand to all 8 lanes. If it fell through to a
// single-lane fallback, this partial load (lanes 4-7) would look independent
// of the vmulf reading $v04.v and could be moved past it.
// (Regression: reorder moved "ldv $v04, 8" below its reader.)
TEST_CASE("Partial load not movable past .v reader", "[reorderPartialLoad]") {
  std::vector<AsmInst> list = {
      asmOp("ldv", {"$v04", "8", "0", "$t6"}),   // 0: writes lanes 4-7
      asmOp("vmulf", {"$v03", "$v05", "$v04.v"}),// 1: reads ALL lanes
      asmOp("or", {"$t0", "$zero", "$zero"}),    // 2
      asmOp("or", {"$t1", "$zero", "$zero"}),    // 3
  };
  for (auto &a : list) asmInitDep(a);
  auto range = asmGetReorderIndices(list, 0);
  bool canMovePastReader =
      std::find(range.begin(), range.end(), 2) != range.end();
  REQUIRE(canMovePastReader == false);
}

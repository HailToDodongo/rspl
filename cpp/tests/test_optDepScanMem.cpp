#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "optimizer/asm_scan_deps.h"
#include "state.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace rspl;

static std::vector<std::vector<int>>
asmLinesToDeps(std::vector<AsmInst> &lines) {
  AsmFunc func;
  func.asm_ = lines;
  asmInitDeps(func);
  lines = std::move(func.asm_);
  std::vector<std::vector<int>> res;
  for (size_t i = 0; i < lines.size(); ++i) {
    auto r = asmGetReorderIndices(lines, static_cast<int>(i));
    std::sort(r.begin(), r.end());
    res.push_back(std::move(r));
  }
  return res;
}

TEST_CASE("Optimizer - Dependency Scanner - Memory - Read vs Read",
          "[optDepScanMem]") {
  std::vector<AsmInst> lines = {
      asmOp("lw", {"$t0", "0($s1)"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("lw", {"$t2", "0($s1)"}),
      asmOp("or", {"$t3", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Memory - Read vs Write",
          "[optDepScanMem]") {
  std::vector<AsmInst> lines = {
      asmOp("lw", {"$t0", "0($s1)"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("sw", {"$t2", "0($s1)"}),
      asmOp("or", {"$t3", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Memory - Read vs Write Barrier",
          "[optDepScanMem]") {
  std::vector<AsmInst> lines = {
      asmOp("lw", {"$t0", "0($s1)"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("sw", {"$t2", "0($s1)"}),
      asmOp("or", {"$t3", "$zero", "$zero"}),
  };
  // Annotate with Barrier
  lines[0].annotations.push_back({"Barrier", "some barrier"});
  lines[2].annotations.push_back({"Barrier", "some barrier"});

  state.reset();
  state.enterFunction("test", "command", 0);
  state.pushScope("", "");

  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1}, {0, 1, 2, 3}, {1, 2, 3}, {0, 1, 2, 3}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Memory - Write vs Write",
          "[optDepScanMem]") {
  std::vector<AsmInst> lines = {
      asmOp("sw", {"$t0", "0($s2)"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("sw", {"$t2", "0($s1)"}),
      asmOp("or", {"$t3", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}, {0, 1, 2, 3}};
  REQUIRE(deps == expected);
}

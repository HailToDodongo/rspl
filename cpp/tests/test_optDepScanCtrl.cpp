#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "optimizer/asm_scan_deps.h"

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

static AsmInst makeLabel(const std::string &name) {
  AsmInst inst;
  inst.type = AsmType::LABEL;
  inst.label = name;
  return inst;
}

TEST_CASE("Optimizer - Dependency Scanner - Control - Stop at Label",
          "[optDepScanCtrl]") {
  std::vector<AsmInst> lines = {
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      makeLabel("SOME_LABEL"),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0, 1}, {0, 1}, {2}, {3}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Control - Stop at Jump",
          "[optDepScanCtrl]") {
  std::vector<AsmInst> lines = {
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("j", {"SOME_WHERE"}),
      asmOp("or", {"$t2", "$zero", "$zero"}), // delay slot (filled)
      asmOp("or", {"$t2", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1}, {0, 1}, {2}, {0, 1, 2, 3}, {4}};
  REQUIRE(deps == expected);
}

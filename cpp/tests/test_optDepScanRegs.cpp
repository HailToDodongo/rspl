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

TEST_CASE("Optimizer - Dependency Scanner - No Deps", "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("or", {"$t2", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0, 1, 2}, {0, 1, 2}, {0, 1, 2}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Basic Write Dep", "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("or", {"$t2", "$t0", "$zero"}),
      asmOp("or", {"$t3", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1}, {0, 1, 2, 3}, {1, 2, 3}, {0, 1, 2, 3}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Nested Write Dep",
          "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("or", {"$t2", "$t0", "$zero"}),
      asmOp("or", {"$t3", "$t2", "$zero"}),
      asmOp("or", {"$t4", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1}, {0, 1, 2, 3, 4}, {1, 2}, {3, 4}, {0, 1, 2, 3, 4}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - MTC2 partial write",
          "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("vxor", {"$v25", "$v00", "$v00.e0"}),
      asmOp("addiu", {"$at", "$zero", "3"}),
      asmOp("mtc2", {"$at", "$v25.e6"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0, 1, 2}, {0, 1}, {2}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - MTC2 partial write no ret",
          "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("vxor", {"$v25", "$v00", "$v00.e0"}),
      asmOp("vxor", {"$v26", "$v00", "$v00"}),
      asmOp("mtc2", {"$at", "$v25.e6"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0, 1, 2}, {1, 2}, {1, 2}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - MTC2 partial write ret regs",
          "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("vxor", {"$v25", "$v00", "$v00.e0"}),
      asmOp("vxor", {"$v26", "$v00", "$v00"}),
      asmOp("mtc2", {"$at", "$v25.e6"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0, 1, 2}, {1, 2}, {1, 2}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Ignore Write no read simple",
          "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t0", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0, 1, 2}, {0, 1, 2}, {2}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Ignore Write no read deps",
          "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t1", "$t0", "$zero"}),
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t0", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0}, {1}, {2}, {3, 4}, {4}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Hidden Regs simple",
          "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("veq", {"$v11", "$v00", "$v00"}),
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("vmrg", {"$v01", "$v02", "$v03"}),
      asmOp("or", {"$t0", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1, 2}, {0, 1, 2, 3, 4}, {0, 1, 2, 3, 4}, {1, 2, 3, 4}, {3, 4}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Regs Single-Lane", "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("vmov", {"$v11.e1", "$v05.e1"}),
      asmOp("vmov", {"$v06.e1", "$v11.e2"}),
      asmOp("vmov", {"$v07.e1", "$v11.e1"}),
      asmOp("vmov", {"$v08.e1", "$v05.e1"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0, 1}, {0, 1, 2, 3}, {1, 2, 3}, {3}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Offset Syntax", "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("or", {"$t0", "$zero", "$zero"}),
      asmOp("or", {"$t1", "$zero", "$zero"}),
      asmOp("lw", {"$t2", "0($t0)"}),
      asmOp("or", {"$t3", "$zero", "$zero"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0, 1}, {0, 1, 2, 3}, {1, 2, 3}, {0, 1, 2, 3}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Vector mul+add", "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("vmudl", {"$v27", "$v18", "$v26.v"}),
      asmOp("vmadm", {"$v27", "$v17", "$v26.v"}),
      asmOp("vmadn", {"$v18", "$v18", "$v25.v"}),
      asmOp("vmadh", {"$v17", "$v17", "$v25.v"}),
      asmOp("vaddc", {"$v18", "$v18", "$v24.v"}),
      asmOp("vadd", {"$v17", "$v17", "$v23.v"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {
      {0}, {1}, {2}, {3}, {4}, {5}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Vector vabs", "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("vmacf", {"$v27", "$v27", "$v27"}),
      asmOp("vabs", {"$v01", "$v01", "$v01"}),
      asmOp("vmacf", {"$v27", "$v27", "$v27"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0}, {1}, {2}};
  REQUIRE(deps == expected);
}

TEST_CASE("Optimizer - Dependency Scanner - Vector VCE", "[optDepScan]") {
  std::vector<AsmInst> lines = {
      asmOp("vcr", {"$v01", "$v01", "$v01"}),
      asmOp("ori", {"$t0", "$t0", "$t0"}),
      asmOp("vcl", {"$v03", "$v03", "$v03"}),
  };
  auto deps = asmLinesToDeps(lines);
  std::vector<std::vector<int>> expected = {{0, 1}, {0, 1, 2}, {1, 2}};
  REQUIRE(deps == expected);
}

#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "optimizer/asm_scan_deps.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace rspl;

static std::vector<std::string> allLanes(const std::string &reg, int start = 0,
                                          int count = 8) {
  std::vector<std::string> r;
  for (int i = 0; i < count; ++i)
    r.push_back(reg + "_" + std::to_string((start + i) % 8));
  return r;
}

static std::string vReg(int n) {
  return std::string("$v") + (n < 10 ? "0" : "") + std::to_string(n);
}

// STV/LTV lanes: 8 consecutive registers starting at base, each at lane
// (8 + i - row) % 8 where row = element_arg / 2.
static std::vector<std::string> stvLanes(int base, int element) {
  int row = element / 2;
  std::vector<std::string> r;
  for (int i = 0; i < 8; ++i)
    r.push_back(vReg(base + i) + "_" +
                std::to_string((8 + i - row) % 8));
  return r;
}

static std::vector<std::string> vecRange(const std::string &, int start,
                                          int count) {
  std::vector<std::string> r;
  for (int i = 0; i < count; ++i)
    r.push_back(vReg(start + i));
  return r;
}

static AsmInst makeAsm(const std::string &op,
                       const std::vector<std::string> &args) {
  AsmInst inst = asmOp(op, args);
  asmInitDep(inst);
  return inst;
}

static auto sorted(std::vector<int> v) {
  std::sort(v.begin(), v.end());
  return v;
}
template <typename C> static auto sorted(const C &c) {
  std::vector<int> v(c.begin(), c.end());
  std::sort(v.begin(), v.end());
  return v;
}

static std::vector<int> idxs(const std::vector<std::string> &regs) {
  std::vector<int> r;
  for (const auto &s : regs) r.push_back(getRegIndex(s));
  return r;
}

static std::vector<int> stallIdxs(const std::vector<std::string> &regs) {
  std::vector<int> r;
  for (const auto &s : regs) r.push_back(getRegStallIndex(s));
  return r;
}

#define VEC(n) "$v" #n

TEST_CASE("Optimizer - Register Scanner", "[optRegScan]") {
  // Logic
  {
    auto a = makeAsm("or", {"$t0", "$a1", "$a0"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({getRegIndex("$a1"), getRegIndex("$a0")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({getRegIndex("$t0")}));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({getRegStallIndex("$a1"), getRegStallIndex("$a0")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({getRegStallIndex("$t0")}));
  }

  // Arith
  {
    auto a = makeAsm("addiu", {"$t0", "$t1", "4"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({getRegIndex("$t1")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({getRegIndex("$t0")}));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({getRegStallIndex("$t1")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({getRegStallIndex("$t0")}));
  }

  // Vec Store
  {
    auto a = makeAsm("sdv", {"$v08", "0", "16", "$s6"});
    auto expSrc = allLanes("$v08");
    expSrc.push_back("$s6");
    auto expSrcIdx = sorted([&]() {
      std::vector<int> r;
      for (auto &s : expSrc) r.push_back(getRegIndex(s));
      return r;
    }());
    auto expStall = std::vector<std::string>{"$v08", "$s6"};
    auto expStallIdx = sorted([&]() {
      std::vector<int> r;
      for (auto &s : expStall) r.push_back(getRegStallIndex(s));
      return r;
    }());
    REQUIRE(sorted(a.depsSourceIdx) == expSrcIdx);
    REQUIRE(sorted(a.depsTargetIdx) == sorted({}));
    REQUIRE(sorted(a.depsStallSourceIdx) == expStallIdx);
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({}));
  }

  // Vec packed Store
  {
    auto a = makeAsm("sfv", {"$v08", "0", "$s6"});
    auto expSrc = allLanes("$v08");
    expSrc.push_back("$s6");
    auto expSrcIdx = sorted([&]() {
      std::vector<int> r;
      for (auto &s : expSrc) r.push_back(getRegIndex(s));
      return r;
    }());
    auto expStall = std::vector<std::string>{"$v08", "$s6"};
    auto expStallIdx = sorted([&]() {
      std::vector<int> r;
      for (auto &s : expStall) r.push_back(getRegStallIndex(s));
      return r;
    }());
    REQUIRE(sorted(a.depsSourceIdx) == expSrcIdx);
    REQUIRE(sorted(a.depsTargetIdx) == sorted({}));
    REQUIRE(sorted(a.depsStallSourceIdx) == expStallIdx);
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({}));
  }

  // Lanes - Vec move
  {
    auto a = makeAsm("vmov", {"$v07.e3", "$v05.e2"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({getRegIndex("$v05_2")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({getRegIndex("$v07_3"), getRegIndex("$acc")}));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({getRegStallIndex("$v05")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({getRegStallIndex("$v07")}));
  }

  // Lanes - STV - base: $v16_0..$v23_7
  {
    auto a = makeAsm("stv", {"$v16", "0", "0", "$t0"});
    auto expSrc = stvLanes(16, 0); // element=0
    expSrc.push_back("$t0");
    auto expSrcIdx = sorted(idxs(expSrc));
    REQUIRE(sorted(a.depsSourceIdx) == expSrcIdx);
    REQUIRE(sorted(a.depsTargetIdx) == sorted({}));
    auto expStall = vecRange("$v16", 16, 8);
    expStall.push_back("$t0");
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted(stallIdxs(expStall)));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({}));
  }

  // Lanes - STV - offset 2: $v08_7, $v09_0..$v15_6
  {
    auto a = makeAsm("stv", {"$v08", "2", "0x10", "$t0"});
    auto expSrc = stvLanes(8, 2);
    expSrc.push_back("$t0");
    REQUIRE(sorted(a.depsSourceIdx) == sorted(idxs(expSrc)));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({}));
    auto expStall = vecRange("$v08", 8, 8);
    expStall.push_back("$t0");
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted(stallIdxs(expStall)));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({}));
  }

  // Lanes - STV - offset 8: $v08_4, $v09_5..$v15_3
  {
    auto a = makeAsm("stv", {"$v08", "8", "0x20", "$t0"});
    auto expSrc = stvLanes(8, 8);
    expSrc.push_back("$t0");
    REQUIRE(sorted(a.depsSourceIdx) == sorted(idxs(expSrc)));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({}));
    auto expStall = vecRange("$v08", 8, 8);
    expStall.push_back("$t0");
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted(stallIdxs(expStall)));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({}));
  }

  // Lanes - LTV - base: $v16_0..$v23_7
  {
    auto a = makeAsm("ltv", {"$v16", "0", "0", "$t0"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({getRegIndex("$t0")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted(idxs(stvLanes(16, 0))));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({getRegStallIndex("$t0")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted(stallIdxs(vecRange("$v16", 16, 8))));
  }

  // Lanes - LTV - offset 2: $v08_7, $v09_0..$v15_6
  {
    auto a = makeAsm("ltv", {"$v08", "2", "0x10", "$t0"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({getRegIndex("$t0")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted(idxs(stvLanes(8, 2))));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({getRegStallIndex("$t0")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted(stallIdxs(vecRange("$v08", 8, 8))));
  }

  // Lanes - LTV - offset 8: $v00_4..$v07_3
  {
    auto a = makeAsm("ltv", {"$v00", "8", "0x20", "$t0"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({getRegIndex("$t0")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted(idxs(stvLanes(0, 8))));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({getRegStallIndex("$t0")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted(stallIdxs(vecRange("$v00", 0, 8))));
  }

  // ctc2 - VCC
  {
    auto a = makeAsm("ctc2", {"$at", "$vcc"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({getRegIndex("$at")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({getRegIndex("$vcc")}));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({getRegStallIndex("$at")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({}));
  }
}

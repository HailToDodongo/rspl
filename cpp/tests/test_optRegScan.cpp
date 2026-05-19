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

template <typename Map>
static std::vector<int> mapRegs(const std::vector<std::string> &regs,
                                const Map &m) {
  std::vector<int> r;
  for (const auto &s : regs) r.push_back(m.at(s));
  return r;
}

static std::vector<int> idxs(const std::vector<std::string> &regs) {
  return mapRegs(regs, REG_INDEX_MAP);
}

static std::vector<int> stallIdxs(const std::vector<std::string> &regs) {
  return mapRegs(regs, REG_STALL_INDEX_MAP);
}

#define VEC(n) "$v" #n

TEST_CASE("Optimizer - Register Scanner", "[optRegScan]") {
  // Logic
  {
    auto a = makeAsm("or", {"$t0", "$a1", "$a0"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({REG_INDEX_MAP.at("$a1"), REG_INDEX_MAP.at("$a0")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({REG_INDEX_MAP.at("$t0")}));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({REG_STALL_INDEX_MAP.at("$a1"), REG_STALL_INDEX_MAP.at("$a0")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({REG_STALL_INDEX_MAP.at("$t0")}));
  }

  // Arith
  {
    auto a = makeAsm("addiu", {"$t0", "$t1", "4"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({REG_INDEX_MAP.at("$t1")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({REG_INDEX_MAP.at("$t0")}));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({REG_STALL_INDEX_MAP.at("$t1")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({REG_STALL_INDEX_MAP.at("$t0")}));
  }

  // Vec Store
  {
    auto a = makeAsm("sdv", {"$v08", "0", "16", "$s6"});
    auto expSrc = allLanes("$v08");
    expSrc.push_back("$s6");
    auto expSrcIdx = sorted([&]() {
      std::vector<int> r;
      for (auto &s : expSrc) r.push_back(REG_INDEX_MAP.at(s));
      return r;
    }());
    auto expStall = std::vector<std::string>{"$v08", "$s6"};
    auto expStallIdx = sorted([&]() {
      std::vector<int> r;
      for (auto &s : expStall) r.push_back(REG_STALL_INDEX_MAP.at(s));
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
      for (auto &s : expSrc) r.push_back(REG_INDEX_MAP.at(s));
      return r;
    }());
    auto expStall = std::vector<std::string>{"$v08", "$s6"};
    auto expStallIdx = sorted([&]() {
      std::vector<int> r;
      for (auto &s : expStall) r.push_back(REG_STALL_INDEX_MAP.at(s));
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
    REQUIRE(sorted(a.depsSourceIdx) == sorted({REG_INDEX_MAP.at("$v05_2")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({REG_INDEX_MAP.at("$v07_3"), REG_INDEX_MAP.at("$acc")}));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({REG_STALL_INDEX_MAP.at("$v05")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({REG_STALL_INDEX_MAP.at("$v07")}));
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
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted(mapRegs(expStall, REG_STALL_INDEX_MAP)));
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
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted(mapRegs(expStall, REG_STALL_INDEX_MAP)));
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
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted(mapRegs(expStall, REG_STALL_INDEX_MAP)));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({}));
  }

  // Lanes - LTV - base: $v16_0..$v23_7
  {
    auto a = makeAsm("ltv", {"$v16", "0", "0", "$t0"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({REG_INDEX_MAP.at("$t0")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted(mapRegs(stvLanes(16, 0), REG_INDEX_MAP)));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({REG_STALL_INDEX_MAP.at("$t0")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted(mapRegs(vecRange("$v16", 16, 8), REG_STALL_INDEX_MAP)));
  }

  // Lanes - LTV - offset 2: $v08_7, $v09_0..$v15_6
  {
    auto a = makeAsm("ltv", {"$v08", "2", "0x10", "$t0"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({REG_INDEX_MAP.at("$t0")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted(mapRegs(stvLanes(8, 2), REG_INDEX_MAP)));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({REG_STALL_INDEX_MAP.at("$t0")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted(mapRegs(vecRange("$v08", 8, 8), REG_STALL_INDEX_MAP)));
  }

  // Lanes - LTV - offset 8: $v00_4..$v07_3
  {
    auto a = makeAsm("ltv", {"$v00", "8", "0x20", "$t0"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({REG_INDEX_MAP.at("$t0")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted(mapRegs(stvLanes(0, 8), REG_INDEX_MAP)));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({REG_STALL_INDEX_MAP.at("$t0")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted(mapRegs(vecRange("$v00", 0, 8), REG_STALL_INDEX_MAP)));
  }

  // ctc2 - VCC
  {
    auto a = makeAsm("ctc2", {"$at", "$vcc"});
    REQUIRE(sorted(a.depsSourceIdx) == sorted({REG_INDEX_MAP.at("$at")}));
    REQUIRE(sorted(a.depsTargetIdx) == sorted({REG_INDEX_MAP.at("$vcc")}));
    REQUIRE(sorted(a.depsStallSourceIdx) == sorted({REG_STALL_INDEX_MAP.at("$at")}));
    REQUIRE(sorted(a.depsStallTargetIdx) == sorted({}));
  }
}

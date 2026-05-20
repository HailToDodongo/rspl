#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

static std::string getDataSection(const std::string &asm_) {
  auto idxData = asm_.find(".data");
  auto idxText = asm_.find(".text");
  if (idxData == std::string::npos || idxText == std::string::npos)
    return "";
  return asm_.substr(idxData, idxText - idxData);
}

TEST_CASE("State - Empty State", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      state {}
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  auto data = getDataSection(result.asm_);
  REQUIRE(data.find("RSPQ_EmptySavedState") != std::string::npos);
}

TEST_CASE("State - Types", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      state {
        u8 a;
        u16 b;
        u32 c;
        vec16 d;
        vec32 e;
      }
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  auto data = getDataSection(result.asm_);
  REQUIRE(data.find("RSPQ_BeginSavedState") != std::string::npos);
  REQUIRE(data.find("STATE_MEM_START:") != std::string::npos);
  REQUIRE(data.find("a: .ds.b 1") != std::string::npos);
  REQUIRE(data.find("b: .ds.b 2") != std::string::npos);
  REQUIRE(data.find("c: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("d: .ds.b 16") != std::string::npos);
  REQUIRE(data.find("e: .ds.b 32") != std::string::npos);
  REQUIRE(data.find("STATE_MEM_END:") != std::string::npos);
}

TEST_CASE("State - Arrays", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      state {
        u32 a0[1];
        u32 a1[4];
        u32 a2[2][4];
        vec32 b0[1];
        vec32 b1[2];
        vec32 b2[4][2];
      }
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  auto data = getDataSection(result.asm_);
  REQUIRE(data.find("a0: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("a1: .ds.b 16") != std::string::npos);
  REQUIRE(data.find("a2: .ds.b 32") != std::string::npos);
  REQUIRE(data.find("b0: .ds.b 32") != std::string::npos);
  REQUIRE(data.find("b1: .ds.b 64") != std::string::npos);
  REQUIRE(data.find("b2: .ds.b 256") != std::string::npos);
}

TEST_CASE("State - Extern", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      state {
        u32 a;
        extern u32 b;
        u32 c;
      }
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  auto data = getDataSection(result.asm_);
  REQUIRE(data.find("a: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("c: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("b:") == std::string::npos);
}

TEST_CASE("State - Align", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      state {
        u16 a;
        alignas(8) u16 b;
        alignas(4) u8 c;
      }
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  auto data = getDataSection(result.asm_);
  REQUIRE(data.find("a: .ds.b 2") != std::string::npos);
  REQUIRE(data.find("b: .ds.b 2") != std::string::npos);
  REQUIRE(data.find("c: .ds.b 1") != std::string::npos);
}

TEST_CASE("State - Data State", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      data {
        u32 BBB;
        u32 CCC;
      }
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  auto data = getDataSection(result.asm_);
  REQUIRE(data.find("RSPQ_EmptySavedState") != std::string::npos);
  REQUIRE(data.find("BBB: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("CCC: .ds.b 4") != std::string::npos);
}

TEST_CASE("State - BSS Only", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      bss {
        u32 DDD;
      }
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_.find(".bss") != std::string::npos);
  REQUIRE(result.asm_.find("DDD: .ds.b 4") != std::string::npos);
}

TEST_CASE("State - Data + State", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      state {
        u32 AAA;
      }
      data {
        u32 BBB;
        u32 CCC;
      }
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  auto data = getDataSection(result.asm_);
  REQUIRE(data.find("AAA: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("BBB: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("CCC: .ds.b 4") != std::string::npos);
}

TEST_CASE("State - Data + State + BSS", "[stateDataBss]") {
  auto result = rspl::transpileSource(
      R"(
      state {
        u32 AAA;
      }
      data {
        u32 BBB;
        u32 CCC;
      }
      bss {
        u32 DDD;
      }
      )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  auto data = getDataSection(result.asm_);
  REQUIRE(data.find("AAA: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("BBB: .ds.b 4") != std::string::npos);
  REQUIRE(data.find("CCC: .ds.b 4") != std::string::npos);
  REQUIRE(result.asm_.find(".bss") != std::string::npos);
  REQUIRE(result.asm_.find("DDD: .ds.b 4") != std::string::npos);
}

TEST_CASE("State - Extern variables are registered for lookup",
          "[stateDataBss]") {
  std::string src = R"(
state {
  extern u32 RDPQ_CMD_STAGING;
  extern u16 RSPQ_Loop;
  vec16 MY_VAR;
}
function test(u32 dummy)
{
  u32 x = RDPQ_CMD_STAGING;
  u32 y = RSPQ_Loop;
}
)";
  auto result = rspl::transpileSource(src, {.rspqWrapper = false});
  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_.find("%lo(RDPQ_CMD_STAGING)") != std::string::npos);
  REQUIRE(result.asm_.find("%lo(RSPQ_Loop)") != std::string::npos);
}

#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

// Global register variables (C++-only extension):
// top-level `u32<$t0> name;` pins a register permanently — visible in every
// function, excluded from auto-allocation, not undef-able.

static void requireThrowsWith(const char *src, const char *msgPart) {
  REQUIRE_THROWS_AS(rspl::transpileSource(src, {.rspqWrapper = false}),
                    std::runtime_error);
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    INFO(e.what());
    REQUIRE(std::string(e.what()).find(msgPart) != std::string::npos);
  }
}

TEST_CASE("GlobalVars - shared across functions", "[globalVars]") {
  auto result = rspl::transpileSource(
      R"(
      u32<$k0> globCounter;

      function fnA() {
        globCounter += 1;
      }

      function fnB() {
        globCounter += 2;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // same pinned register in both functions
  REQUIRE(result.asm_.find("fnA:") != std::string::npos);
  REQUIRE(result.asm_.find("fnB:") != std::string::npos);
  REQUIRE(result.asm_.find("addiu $k0, $k0, 1") != std::string::npos);
  REQUIRE(result.asm_.find("addiu $k0, $k0, 2") != std::string::npos);
}

TEST_CASE("GlobalVars - excluded from auto-allocation", "[globalVars]") {
  // $t0 is pinned, so the first auto-allocated scalar must be $t1
  auto result = rspl::transpileSource(
      R"(
      u32<$t0> pinned;

      function test() {
        u32 a;
        a += 1;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $t1, $t1, 1
  jr $ra
  nop)");
}

TEST_CASE("GlobalVars - vector global usable in ops", "[globalVars]") {
  // $v01 pinned -> auto-allocated vector goes to $v02
  auto result = rspl::transpileSource(
      R"(
      vec16<$v01> globVec;

      function test() {
        vec16 v;
        v = globVec +* globVec.x;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadn $v02, $v01, $v01.e0
  jr $ra
  nop)");
}

TEST_CASE("GlobalVars - vec32 pins two registers", "[globalVars]") {
  // vec32 on $v01 occupies $v01+$v02 -> auto-alloc starts at $v03
  auto result = rspl::transpileSource(
      R"(
      vec32<$v01> globVec;

      function test() {
        vec16 v;
        v = v +* v.x;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadn $v03, $v03, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("GlobalVars - undef is rejected", "[globalVars]") {
  requireThrowsWith(
      R"(
      u32<$t0> pinned;

      function test() {
        undef pinned;
      })",
      "Cannot undef global register variable 'pinned'");
}

TEST_CASE("GlobalVars - local explicit register collides", "[globalVars]") {
  requireThrowsWith(
      R"(
      u32<$t0> pinned;

      function test() {
        u32<$t0> x;
      })",
      "Register '$t0' already used for variable 'pinned'");
}

TEST_CASE("GlobalVars - command arg register collides", "[globalVars]") {
  // implicit command args land in $a0.. — pinning one of those collides
  requireThrowsWith(
      R"(
      u32<$a0> pinned;

      command<0> test(u32 arg) {
        arg += 1;
      })",
      "Register '$a0' already used for variable 'pinned'");
}

TEST_CASE("GlobalVars - built-in register collides", "[globalVars]") {
  requireThrowsWith(
      R"(
      vec16<$v30> pinned;

      function test() {})",
      "Register '$v30' already used for variable 'VSHIFT'");
}

TEST_CASE("GlobalVars - duplicate declaration", "[globalVars]") {
  requireThrowsWith(
      R"(
      u32<$t0> pinned;
      u32<$t1> pinned;

      function test() {})",
      "Global variable 'pinned' already declared");
}

TEST_CASE("GlobalVars - const global rejects writes", "[globalVars]") {
  requireThrowsWith(
      R"(
      const u32<$t0> pinned;

      function test() {
        pinned += 1;
      })",
      "const");
}

TEST_CASE("GlobalVars - parse errors", "[globalVars]") {
  // register is mandatory
  requireThrowsWith(
      R"(
      u32 pinned;

      function test() {})",
      "must specify a register");
  // one name per declaration
  requireThrowsWith(
      R"(
      u32<$t0> a, b;

      function test() {})",
      "one per statement");
  // no initializers at file scope
  requireThrowsWith(
      R"(
      u32<$t0> a = 1;

      function test() {})",
      "cannot have an initializer");
}

TEST_CASE("GlobalVars - interleaved with state section", "[globalVars]") {
  auto result = rspl::transpileSource(
      R"(
      u32<$k1> globA;

      state { u32 SOME_VALUE; }

      vec16<$v20> globB;

      function test() {
        globA += 1;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_.find("addiu $k1, $k1, 1") != std::string::npos);
}

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "pipeline.h"

TEST_CASE("Control - Exit", "[control]") {
  auto result = rspl::transpileSource(
      R"(function test()
{
  exit;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  j RSPQ_Loop
  nop
  jr $ra
  nop)");
}

TEST_CASE("Control - Unlikely If", "[control]") {
  auto result = rspl::transpileSource(
      R"(state { u32 FOO; }
function test()
{
  u32<$t0> a = load(FOO);
  @Unlikely if(a != 0)
  {
    a += 1;
    store(a, FOO);
  }
  a += 2;
  store(a, FOO);
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  lw $t0, %lo(FOO + 0)
  bne $t0, $zero, LABEL_test_0001
  nop
  LABEL_test_0002:
  addiu $t0, $t0, 2
  sw $t0, %lo(FOO)($zero)
  jr $ra
  nop
  LABEL_test_0001:
  addiu $t0, $t0, 1
  sw $t0, %lo(FOO)($zero)
  j LABEL_test_0002
  nop)");
}

TEST_CASE("Control - Unlikely If (slt compare)", "[control]") {
  auto result = rspl::transpileSource(
      R"(state { u32 FOO; }
function test()
{
  u32<$t0> a = load(FOO);
  u32<$t1> b = load(FOO);
  @Unlikely if(a < b)
  {
    a += 1;
  }
  store(a, FOO);
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  lw $t0, %lo(FOO + 0)
  lw $t1, %lo(FOO + 0)
  sltu $at, $t0, $t1
  bne $at, $zero, LABEL_test_0001
  nop
  LABEL_test_0002:
  sw $t0, %lo(FOO)($zero)
  jr $ra
  nop
  LABEL_test_0001:
  addiu $t0, $t0, 1
  j LABEL_test_0002
  nop)");
}

TEST_CASE("Control - Unlikely If - else not allowed", "[control]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(state { u32 FOO; }
function test()
{
  u32<$t0> a = load(FOO);
  @Unlikely if(a != 0) {
    a += 1;
  } else {
    a += 2;
  }
  store(a, FOO);
})",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("else-block"));
}

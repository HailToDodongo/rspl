#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

TEST_CASE("Syntax - Numbers - Scalar Assignment", "[syntaxNumbers]") {
  auto result = rspl::transpileSource(
      R"(function test()
{
  u32<$t0> a;
  a = 1234;
  a = 0x1234;
  a = 0b1010;
})",
      {.rspqWrapper = false});

  REQUIRE(result.asm_ == R"(test:
  addiu $t0, $zero, 1234
  addiu $t0, $zero, 4660
  addiu $t0, $zero, 10
  jr $ra
  nop)");
}

TEST_CASE("Syntax - Numbers - Scalar Calc", "[syntaxNumbers]") {
  auto result = rspl::transpileSource(
      R"(function test()
{
  u32<$t0> a;
  a = a + 1234;
  a = a + 0x1234;
  a = a + 0b1010;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $t0, $t0, 1234
  addiu $t0, $t0, 4660
  addiu $t0, $t0, 10
  jr $ra
  nop)");
}

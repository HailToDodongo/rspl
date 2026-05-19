#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

static rspl::TranspileResult optTranspile(const std::string &src) {
  return rspl::transpileSource(src, {.rspqWrapper = false, .optimize = true});
}

TEST_CASE("Optimizer E2E - Dead Code - Jump at end - safe", "[optDeadCode]") {
  auto res = optTranspile(R"(function test()
{
  goto TEST;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  j TEST
  nop)");
}

TEST_CASE("Optimizer E2E - Dead Code - Jump at end - unsafe with code", "[optDeadCode]") {
  auto res = optTranspile(R"(function test()
{
  goto TEST;
  u32 x = 2;
  x = 3;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  j TEST
  nop
  addiu $t0, $zero, 3
  jr $ra
  addiu $t0, $zero, 2)");
}

TEST_CASE("Optimizer E2E - Dead Code - Jump at end - unsafe", "[optDeadCode]") {
  auto res = optTranspile(R"(
function test2();
function test()
{
  test2();
  u32 x = 2;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  jal test2
  nop
  jr $ra
  addiu $t0, $zero, 2)");
}

TEST_CASE("Optimizer E2E - Dead Code - Jump in branch - safe jal", "[optDeadCode]") {
  auto res = optTranspile(R"(
function test2();
function test()
{
  u32 x = 1;
  if(x) {
    test2();
  }
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  addiu $t0, $zero, 1
  bne $t0, $zero, test2
  ori $ra, $zero, LABEL_test_0001
  LABEL_test_0001:
  jr $ra
  nop)");
}

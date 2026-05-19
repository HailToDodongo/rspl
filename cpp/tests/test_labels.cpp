#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

TEST_CASE("Labels - Basic", "[labels]") {
  auto result = rspl::transpileSource(
      R"(
function test_label()
{
  label_a:
  label_b: label_c:
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_label:
  label_a:
  label_b:
  label_c:
  jr $ra
  nop)");
}

TEST_CASE("Labels - With instructions", "[labels]") {
  auto result = rspl::transpileSource(
      R"(
function test_label()
{
  u32<$t0> a;
  label_a:
    a += 1;
    goto label_b;
  label_b:
    a += 2;
    goto label_a;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_label:
  label_a:
  addiu $t0, $t0, 1
  j label_b
  nop
  label_b:
  addiu $t0, $t0, 2
  j label_a
  nop
  jr $ra
  nop)");
}

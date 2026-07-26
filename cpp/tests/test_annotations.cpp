#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

TEST_CASE("Annotations - Align (function)", "[annotations]") {
  auto result = rspl::transpileSource(
      R"(
@Align(8)
function test()
{
  exit;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(.align 3
test:
  j RSPQ_Loop
  nop
  jr $ra
  nop)");
}

TEST_CASE("Annotations - Relative (function)", "[annotations]") {
  auto result = rspl::transpileSource(
      R"(
@Relative
function target_rel() {}
function target_abs() {}
function caller() {
  target_rel();
  target_abs();
}
)",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(target_rel:
  jr $ra
  nop
target_abs:
  jr $ra
  nop
caller:
  bgezal $zero, target_rel
  nop
  jal target_abs
  nop
  jr $ra
  nop)");
}

TEST_CASE("Annotations - Relative (caller)", "[annotations]") {
  auto result = rspl::transpileSource(
      R"(
function target_rel() {}
function target_abs() {}
function caller() {
  @Relative target_rel();
  target_abs();
}
)",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(target_rel:
  jr $ra
  nop
target_abs:
  jr $ra
  nop
caller:
  bgezal $zero, target_rel
  nop
  jal target_abs
  nop
  jr $ra
  nop)");
}

TEST_CASE("Annotations - Tag", "[annotations]") {
  auto result = rspl::transpileSource(
      R"(function test_tag() {
    u32<$t0> a;
    @Tag("TEST") a = 1;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_tag:
  TAG_TEST: addiu $t0, $zero, 1
  jr $ra
  nop)");
}

// The annotation sits on the declaration but must reach the generated
// assignment instruction.
TEST_CASE("Annotations - Tag (declaration-assignment)", "[annotations]") {
  auto result = rspl::transpileSource(
      R"(function test_tag() {
    @Tag("TEST") u32<$t0> a = 1;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_tag:
  TAG_TEST: addiu $t0, $zero, 1
  jr $ra
  nop)");
}

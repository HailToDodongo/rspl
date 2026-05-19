#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Macros - Basic replacement", "[macros]") {
  auto result = rspl::transpileSource(
      R"(
      macro test(u32 add) {
        add += 42;
      }

      function test_macro() {
        u32<$t2> a;
        u32<$s3> b;
        test(a);

        if(a < 3) {
          test(a);
        }
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_macro:
  addiu $t2, $t2, 42
  sltiu $at, $t2, 3
  beq $at, $zero, LABEL_test_macro_0001
  nop
  addiu $t2, $t2, 42
  LABEL_test_macro_0001:
  jr $ra
  nop)");
}

TEST_CASE("Macros - Nested macro", "[macros]") {
  auto result = rspl::transpileSource(
      R"(
      macro test_b(u32 argB) {
        argB += 42;
      }

      macro test_a(u32 argA) {
        test_b(argA);
      }

      function test_macro() {
        u32<$t2> a;
        test_a(a);
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_macro:
  addiu $t2, $t2, 42
  jr $ra
  nop)");
}

TEST_CASE("Macros - Scope local", "[macros]") {
  auto result = rspl::transpileSource(
      R"(
      macro test_b(u32 argB) {
        argB += 42;
      }

      function test_macro() {
        u32<$t2> a;
        u32<$t3> argB;
        test_b(a);
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_macro:
  addiu $t2, $t2, 42
  jr $ra
  nop)");
}

TEST_CASE("Macros - Return Value", "[macros]") {
  auto result = rspl::transpileSource(
      R"(
      macro test_a(u32 res, u32 argA, u32 argB) {
        res = argA + argB;
      }

      function test_macro() {
        u32<$a0> argA, argB;
        u32<$s0> a = test_a(argA, argB);
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_macro:
  addu $s0, $a0, $a1
  jr $ra
  nop)");
}

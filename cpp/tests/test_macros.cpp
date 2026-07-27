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

// --- Local macros (C++-only extension) --------------------------------

TEST_CASE("Macros - Local basic", "[macros]") {
  auto result = rspl::transpileSource(
      R"(
      function test_local() {
        macro inc(u32 add) {
          add += 42;
        }
        u32<$t2> a;
        inc(a);

        if(a < 3) {
          inc(a);
        }
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_local:
  addiu $t2, $t2, 42
  sltiu $at, $t2, 3
  beq $at, $zero, LABEL_test_local_0001
  nop
  addiu $t2, $t2, 42
  LABEL_test_local_0001:
  jr $ra
  nop)");
}

TEST_CASE("Macros - Local sees call-site scope", "[macros]") {
  // 'base' is declared before the macro, 'later' after it but before the
  // call: both must resolve, since the body binds at the calling site.
  auto result = rspl::transpileSource(
      R"(
      function test_capture() {
        u32<$t0> base;
        macro addBoth(u32 dst) {
          dst += base;
          dst += later;
        }
        u32<$t1> later;
        u32<$t2> a;
        addBoth(a);
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_capture:
  addu $t2, $t2, $t0
  addu $t2, $t2, $t1
  jr $ra
  nop)");
}

TEST_CASE("Macros - Local de-phasing (multiple inlinings)", "[macros]") {
  auto result = rspl::transpileSource(
      R"(
      function test_dephase() {
        macro step(u32 v) {
          v += 1;
        }
        u32<$t0> a;
        u32<$t1> b;
        step(a);
        step(b);
        step(a);
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_dephase:
  addiu $t0, $t0, 1
  addiu $t1, $t1, 1
  addiu $t0, $t0, 1
  jr $ra
  nop)");
}

TEST_CASE("Macros - Local shadows global", "[macros]") {
  auto result = rspl::transpileSource(
      R"(
      macro doOp(u32 v) {
        v += 1;
      }

      function test_shadow() {
        u32<$t0> a;
        doOp(a);
        macro doOp(u32 v) {
          v += 2;
        }
        doOp(a);
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_shadow:
  addiu $t0, $t0, 1
  addiu $t0, $t0, 2
  jr $ra
  nop)");
}

TEST_CASE("Macros - Local return value", "[macros]") {
  auto result = rspl::transpileSource(
      R"(
      function test_ret() {
        macro sum(u32 res, u32 x, u32 y) {
          res = x + y;
        }
        u32<$a0> argA, argB;
        u32<$s0> a = sum(argA, argB);
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_ret:
  addu $s0, $a0, $a1
  jr $ra
  nop)");
}

TEST_CASE("Macros - Local dies with enclosing block", "[macros]") {
  const char *src = R"(
      function test_scope_end() {
        u32<$t0> a;
        {
          macro inc(u32 v) { v += 1; }
          inc(a);
        }
        inc(a);
      })";
  REQUIRE_THROWS_AS(rspl::transpileSource(src, {.rspqWrapper = false}),
                    std::runtime_error);
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find("Function inc not known") !=
            std::string::npos);
  }
}

TEST_CASE("Macros - Local not visible before declaration", "[macros]") {
  const char *src = R"(
      function test_before() {
        u32<$t0> a;
        inc(a);
        macro inc(u32 v) { v += 1; }
      })";
  REQUIRE_THROWS_AS(rspl::transpileSource(src, {.rspqWrapper = false}),
                    std::runtime_error);
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find("Function inc not known") !=
            std::string::npos);
  }
}

TEST_CASE("Macros - Local parse errors", "[macros]") {
  // only macros may be nested
  REQUIRE_THROWS_AS(rspl::transpileSource(
                        R"(function outer() {
                          function inner() {}
                        })",
                        {.rspqWrapper = false}),
                    std::runtime_error);
  // no result-type on macros
  REQUIRE_THROWS_AS(rspl::transpileSource(
                        R"(function outer() {
                          macro m<0x1>() {}
                        })",
                        {.rspqWrapper = false}),
                    std::runtime_error);
  // no forward declarations inside a function
  REQUIRE_THROWS_AS(rspl::transpileSource(
                        R"(function outer() {
                          macro m();
                        })",
                        {.rspqWrapper = false}),
                    std::runtime_error);
}

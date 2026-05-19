#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Comparison - Vector (vec16 vs vec16)", "[compare]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a < b;
      res = a >= b;
      res = a == b;
      res = a != b;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vlt $v01, $v02, $v03
  vge $v01, $v02, $v03
  veq $v01, $v02, $v03
  vne $v01, $v02, $v03
  jr $ra
  nop)");
}

TEST_CASE("Comparison - Vector (vec16 vs const)", "[compare]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a < 0;
      res = a >= 2;
      res = a == 32;
      res = a != 256;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vlt $v01, $v02, $v00.e0
  vge $v01, $v02, $v30.e6
  veq $v01, $v02, $v30.e2
  vne $v01, $v02, $v31.e7
  jr $ra
  nop)");
}

TEST_CASE("Comparison - Vector-Select (vec16)", "[compare]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = select(a, b);
      res = select(a, 32);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmrg $v01, $v02, $v03
  vmrg $v01, $v02, $v30.e2
  jr $ra
  nop)");
}

TEST_CASE("Comparison - Vector-Select (vec32)", "[compare]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a, b;
      A:
      res = select(a, b);
      B:
      res = select(a, b.y);
      C:
      res = select(a, 32);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  A:
  vmrg $v01, $v03, $v05
  vmrg $v02, $v04, $v06
  B:
  vmrg $v01, $v03, $v05.e1
  vmrg $v02, $v04, $v06.e1
  C:
  vmrg $v01, $v03, $v30.e2
  vmrg $v02, $v04, $v00.e2
  jr $ra
  nop)");
}

TEST_CASE("Comparison - Vector-Select (vec32 cast)", "[compare]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a, b;

      res:sint = select(a, b:sfract);
      res:sfract = select(a, 32);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmrg $v01, $v03, $v06
  vmrg $v02, $v04, $v00.e2
  jr $ra
  nop)");
}

TEST_CASE("Comparison - Vector-Ternary (vec16)", "[compare]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      vec16<$v10> x, y;

      A:
      res = x != y ? a : b;
      B:
      res = x != 4 ? a : 32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  A:
  vne $v29, $v10, $v11
  vmrg $v01, $v02, $v03
  B:
  vne $v29, $v10, $v30.e5
  vmrg $v01, $v02, $v30.e2
  jr $ra
  nop)");
}

TEST_CASE("Comparison - Vector-Ternary (vec32)", "[compare]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a, b;
      vec16<$v10> x, y;

      A:
      res = x != y ? a : b;
      B:
      res = x != 4 ? a : 32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  A:
  vne $v29, $v10, $v11
  vmrg $v01, $v03, $v05
  vmrg $v02, $v04, $v06
  B:
  vne $v29, $v10, $v30.e5
  vmrg $v01, $v03, $v30.e2
  vmrg $v02, $v04, $v00.e2
  jr $ra
  nop)");
}

TEST_CASE("Comparison - Vector-Ternary (swizzle)", "[compare]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      A:
      res = a == b ? a : b.y;
      B:
      res = a >= b.z ? a : b.y;
      C:
      res = a == b.z ? a : b;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  A:
  veq $v29, $v02, $v03
  vmrg $v01, $v02, $v03.e1
  B:
  vge $v29, $v02, $v03.e2
  vmrg $v01, $v02, $v03.e1
  C:
  veq $v29, $v02, $v03.e2
  vmrg $v01, $v02, $v03
  jr $ra
  nop)");
}

#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Syntax - Swizzle - Assign single (vec32 <- vec32)", "[swizzle]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> a, b;
      a.x = b.X;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmov $v01.e0, $v03.e4
  vmov $v02.e0, $v04.e4
  jr $ra
  nop)");
}

TEST_CASE("Syntax - Swizzle - Assign single (vec32 <- vec32, cast)",
          "[swizzle]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> a, b;
      SINT:
      a.x = b:sint.X;
      a:sint.x = b:sint.X;
      a:ufract.x = b:sint.X;

      UFRACT:
      a.x = b:ufract.X;
      a:sint.x = b:ufract.X;
      a:ufract.x = b:ufract.X;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  SINT:
  vmov $v01.e0, $v03.e4
  vmov $v02.e0, $v00.e4
  vmov $v01.e0, $v03.e4
  vmov $v02.e0, $v03.e4
  UFRACT:
  vmov $v01.e0, $v00.e4
  vmov $v02.e0, $v04.e4
  vmov $v01.e0, $v04.e4
  vmov $v02.e0, $v04.e4
  jr $ra
  nop)");
}

TEST_CASE("Syntax - Swizzle - Assign single (vec16 <- vec16)", "[swizzle]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a, b;
      a.x = b.X;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmov $v01.e0, $v02.e4
  jr $ra
  nop)");
}

TEST_CASE("Syntax - Swizzle - Assign single (vec32 <- vec16)", "[swizzle]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> a;
      vec16<$v03> b;
      a.x = b.X;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmov $v01.e0, $v03.e4
  vmov $v02.e0, $v00.e4
  jr $ra
  nop)");
}

TEST_CASE("Syntax - Swizzle - Assign single (vec16 <- vec32)", "[swizzle]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a.x = b.X;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmov $v01.e0, $v02.e4
  jr $ra
  nop)");
}

TEST_CASE("Syntax - Swizzle - Invalid on Scalar (calc)", "[swizzle]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test() {
      u32<$t0> a;
      a += a.x;
    })",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test() {
      u32<$t0> a;
      a += a.x;
    })",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Swizzling not allowed for scalar operations") != std::string::npos);
  }
}

TEST_CASE("Syntax - Swizzle - Invalid on Scalar (assign)", "[swizzle]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test() {
      u32<$t0> a;
      a = a.x;
    })",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test() {
      u32<$t0> a;
      a = a.x;
    })",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Swizzling not allowed for scalar operations") != std::string::npos);
  }
}

TEST_CASE("Syntax - Swizzle - Alias (integer index)", "[swizzle]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a;
      a.x = a.0;
      a.1 = a.z;

      a += a.xxzzXXZZ;
      a += a.00224466;

      a += a.wwwwWWWW;
      a += a.33337777;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmov $v01.e0, $v01.e0
  vmov $v01.e1, $v01.e2
  vaddc $v01, $v01, $v01.q0
  vaddc $v01, $v01, $v01.q0
  vaddc $v01, $v01, $v01.h3
  vaddc $v01, $v01, $v01.h3
  jr $ra
  nop)");
}

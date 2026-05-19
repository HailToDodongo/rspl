#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Scope - Var Declaration", "[scope]") {
  auto result = rspl::transpileSource(
      R"(function test_scope()
{
  u32<$t0> a;
  {
     u32<$t1> b;
     b += 2;
  } // 'b' is no longer defined now
  a += 2;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_scope:
  addiu $t1, $t1, 2
  addiu $t0, $t0, 2
  jr $ra
  nop)");
}

TEST_CASE("Scope - Var Un-Declaration", "[scope]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test_scope()
{
  u32<$t0> a;
  a += 2;
  undef a;
  a = 2;
})",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test_scope()
{
  u32<$t0> a;
  a += 2;
  undef a;
  a = 2;
})",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find("Variable a not known") != std::string::npos);
  }
}

TEST_CASE("Scope - Var Decl. invalid", "[scope]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test_scope()
{
  u32<$t0> a;
  {
     u32<$t1> b;
     b += 2;
  }
  b += 2;
})",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test_scope()
{
  u32<$t0> a;
  {
     u32<$t1> b;
     b += 2;
  }
  b += 2;
})",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find("Variable b not known") != std::string::npos);
  }
}

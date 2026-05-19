#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Const - Declaration", "[const]") {
  auto result = rspl::transpileSource(
      R"(function test()
{
  const u32<$t0> a = 1234;
  const u32<$t1> b = a + a;
  const vec16<$v01> c = 0;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $t0, $zero, 1234
  addu $t1, $t0, $t0
  vxor $v01, $v00, $v00.e0
  jr $ra
  nop)");
}

TEST_CASE("Const - Invalid (scalar reassignment)", "[const]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test() {
  const u32<$t0> a = 1234;
  a += 1;
})",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test() {
  const u32<$t0> a = 1234;
  a += 1;
})",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Cannot assign to constant variable") != std::string::npos);
  }
}

TEST_CASE("Const - Invalid (vector reassignment)", "[const]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test() {
  const vec16<$v01> a = 0;
  a += a;
})",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test() {
  const vec16<$v01> a = 0;
  a += a;
})",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Cannot assign to constant variable") != std::string::npos);
  }
}

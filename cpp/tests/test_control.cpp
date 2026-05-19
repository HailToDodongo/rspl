#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

TEST_CASE("Control - Exit", "[control]") {
  auto result = rspl::transpileSource(
      R"(function test()
{
  exit;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  j RSPQ_Loop
  nop
  jr $ra
  nop)");
}

#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

static rspl::TranspileResult optTranspile(const std::string &src) {
  return rspl::transpileSource(src, {.rspqWrapper = false, .optimize = true});
}

TEST_CASE("Optimizer E2E - Labels - De-dupe Labels", "[optLabels]") {
  auto res = optTranspile(R"(function test(u32 dummy)
{
  LABEL_A:
  LABEL_B:
  LABEL_C:
  goto LABEL_A;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  LABEL_C:
  j LABEL_C
  nop)");
}

TEST_CASE("Optimizer E2E - Labels - De-dupe Labels - keep single", "[optLabels]") {
  auto res = optTranspile(R"(function test(u32 dummy)
{
  LABEL_A:
  dummy += 1;
  LABEL_B:
  dummy += 2;
  LABEL_C:
  goto LABEL_A;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  LABEL_A:
  addiu $a0, $a0, 1
  LABEL_B:
  addiu $a0, $a0, 2
  LABEL_C:
  j LABEL_A
  nop)");
}

#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

static rspl::TranspileResult optTranspile(const std::string &src) {
  return rspl::transpileSource(src, {.rspqWrapper = false, .optimize = true});
}

TEST_CASE("Optimizer E2E - Jump Dedupe - Nested-If Used Label", "[optJumpDedupe]") {
  auto res = optTranspile(R"(command<0> test()
{
  u32 a = 1;
  if(a > 1) {
    if(a > 10) {
      a += 1;
    }
  }
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  addiu $s7, $zero, 1
  sltiu $at, $s7, 2
  bne $at, $zero, RSPQ_Loop
  nop
  sltiu $at, $s7, 11
  bne $at, $zero, RSPQ_Loop
  nop
  addiu $s7, $s7, 1
  LABEL_test_0001:
  j RSPQ_Loop
  nop)");
}

TEST_CASE("Optimizer E2E - Jump Dedupe - Nested-If Unused Label", "[optJumpDedupe]") {
  auto res = optTranspile(R"(command<0> test()
{
  u32 a = 1;
  while(a < 2) {
    a -= 1;
  }
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  addiu $s7, $zero, 1
  LABEL_test_0001:
  sltiu $at, $s7, 2
  beq $at, $zero, RSPQ_Loop
  nop
  j LABEL_test_0001
  addiu $s7, $s7, 65535)");
}

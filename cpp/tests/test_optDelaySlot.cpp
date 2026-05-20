#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

static rspl::TranspileResult optTranspile(const std::string &src) {
  return rspl::transpileSource(src, {.rspqWrapper = false, .optimize = true});
}

TEST_CASE("Optimizer E2E - Delay-Slots - Fill - Basic", "[optDelaySlot]") {
  auto res = optTranspile(R"(function test(u32 dummy)
{
  u32 a = 1;
  goto SOME_LABEL;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  j SOME_LABEL
  addiu $t0, $zero, 1)");
}

TEST_CASE("Optimizer E2E - Delay-Slots - Fill - Complex", "[optDelaySlot]") {
  auto res = optTranspile(R"(function test(u32 i)
{
  u32 test = 0;
  while(i != 0) {
    if(i == 6) {
      test = 42;
      break;
    }
    i -= 1;
  }
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  or $t0, $zero, $zero
  LABEL_test_0001:
  beq $a0, $zero, LABEL_test_0002
  nop
  addiu $at, $zero, 6
  bne $a0, $at, LABEL_test_0003
  nop
  j LABEL_test_0002
  addiu $t0, $zero, 42
  LABEL_test_0003:
  j LABEL_test_0001
  addiu $a0, $a0, 65535
  LABEL_test_0002:
  jr $ra
  nop)");
}

TEST_CASE("Optimizer E2E - Delay-Slots - Fill across jal (scalar)",
          "[optDelaySlot]") {
  auto res = optTranspile(R"(
function DMAWaitIdle();
function test()
{
  u32 a = 1;
  u32 b = 2;
  DMAWaitIdle();
  u32 c = 3;
}
)");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  addiu $t1, $zero, 2
  jal DMAWaitIdle
  addiu $t0, $zero, 1
  jr $ra
  addiu $t2, $zero, 3)");
}

#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

static rspl::TranspileResult optTranspile(const std::string &src) {
  return rspl::transpileSource(src, {.rspqWrapper = false, .optimize = true});
}

TEST_CASE("Optimizer E2E - Branch-Jump - Branch + Goto", "[optBranchJump]") {
  auto res = optTranspile(R"(function test()
{
  u32<$t0> a;
  LABEL_A:
  if(a != 0)goto LABEL_A;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  LABEL_A:
  bne $t0, $zero, LABEL_A
  nop
  jr $ra
  nop)");
}

TEST_CASE("Optimizer E2E - Branch-Jump - Branch + Goto (no opt)", "[optBranchJump]") {
  auto res = optTranspile(R"(function test()
{
  u32<$t0> a;
  LABEL_A:
  if(a != 0) {
    a += 1;
    goto LABEL_A;
  }
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  LABEL_A:
  beq $t0, $zero, LABEL_test_0001
  nop
  j LABEL_A
  addiu $t0, $t0, 1
  LABEL_test_0001:
  jr $ra
  nop)");
}

TEST_CASE("Optimizer E2E - Branch-Jump - Loop - Used Label", "[optBranchJump]") {
  auto res = optTranspile(R"(function test()
{
  u32<$t0> a;
  loop {
    if(a == 1)continue;
    SOME_LABEL:

    if(a == 0)goto SOME_LABEL;
    LOOP_END:
  }
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  LABEL_test_0001:
  addiu $at, $zero, 1
  beq $t0, $at, LABEL_test_0001
  nop
  SOME_LABEL:
  bne $t0, $zero, LABEL_test_0001
  nop
  j SOME_LABEL
  nop
  LABEL_test_0002:
  jr $ra
  nop)");
}

TEST_CASE("Optimizer E2E - Branch-Jump - Loop - Unused Label", "[optBranchJump]") {
  auto res = optTranspile(R"(function test()
{
  u32<$t0> a;
  loop {
    if(a == 1)continue;
    SOME_LABEL:

    if(a == 0)continue;
    LOOP_END:
  }
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  LABEL_test_0001:
  addiu $at, $zero, 1
  beq $t0, $at, LABEL_test_0001
  nop
  bne $t0, $zero, LABEL_test_0001
  nop
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop)");
}

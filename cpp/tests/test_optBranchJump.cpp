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
  // LOOP_END is a user label: it stays (with the loop's own back-jump
  // behind it), only the compiler's labels get folded away
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
  LOOP_END:
  j LABEL_test_0001
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
  // user labels are kept even when nothing in this function targets them:
  // another function or hand-written assembly may
  REQUIRE(res.asm_ == R"(test:
  LABEL_test_0001:
  addiu $at, $zero, 1
  beq $t0, $at, LABEL_test_0001
  nop
  SOME_LABEL:
  bne $t0, $zero, LABEL_test_0001
  nop
  j LABEL_test_0001
  nop
  LOOP_END:
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop)");
}

// Regression: "if(..) goto X;" directly followed by a user label. The
// compiler's else-label and the user label are adjacent, dedupeLabels
// folds the else-label into the user one, and branchJump then saw a branch
// to a label nothing else in *this* function used and deleted it — even
// though another function jumps there.
TEST_CASE("Optimizer E2E - Branch-Jump - user label after if-goto survives", "[optBranchJump]") {
  auto res = optTranspile(R"(state { extern u16 RSPQ_Loop; }
function other()
{
  goto L_B;
}
function test()
{
  u32<$t0> a;
  u32<$t1> b;
  L_A:
    a += 1;
    if(a == b) goto RSPQ_Loop;
  L_B:
  {
    a += 2;
  }
  goto L_A;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(other:
  j L_B
  nop
test:
  L_A:
  addiu $t0, $t0, 1
  beq $t0, $t1, RSPQ_Loop
  nop
  L_B:
  j L_A
  addiu $t0, $t0, 2)");
}

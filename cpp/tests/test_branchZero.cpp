#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Branch (Var vs. 0) - Equal - U32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a == 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bne $v0, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Not-Equal - U32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a != 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  beq $v0, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Greater - U32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a > 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  blez $v0, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Less - U32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a < 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bgez $v0, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Greater-Equal - U32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a >= 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bltz $v0, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Less-Equal - U32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a <= 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bgtz $v0, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

// Signed

TEST_CASE("Branch (Var vs. 0) - Equal - S32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a == 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bne $v0, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Not-Equal - S32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a != 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  beq $v0, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Greater - S32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a > 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  blez $v0, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Less - S32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a < 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bgez $v0, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Greater-Equal - S32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a >= 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bltz $v0, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

TEST_CASE("Branch (Var vs. 0) - Less-Equal - S32", "[branchZero]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a <= 0) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bgtz $v0, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop)");
}

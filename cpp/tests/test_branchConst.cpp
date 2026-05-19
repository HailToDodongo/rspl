#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Branch (Var vs. Const) - Equal - U32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a == 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  addiu $at, $zero, 42
  bne $v0, $at, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Equal - U32 (big number)",
          "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a == 0x112233) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  lui $at, 0x11
  ori $at, $at, 0x2233
  bne $v0, $at, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Not-Equal - U32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a != 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  addiu $at, $zero, 42
  beq $v0, $at, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Greater - U32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a > 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  sltiu $at, $v0, 43
  bne $at, $zero, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Greater - U32 (big number)",
          "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a > 0xFFFEFFFF) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  lui $at, 0xFFFF
  sltu $at, $v0, $at
  bne $at, $zero, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Less - U32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a < 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  sltiu $at, $v0, 42
  beq $at, $zero, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Greater-Than - U32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a >= 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  sltiu $at, $v0, 42
  bne $at, $zero, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Less-Than - U32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a;
      if(a <= 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  sltiu $at, $v0, 43
  beq $at, $zero, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Equal - S32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a == 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  addiu $at, $zero, 42
  bne $v0, $at, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Not-Equal - S32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a != 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  addiu $at, $zero, 42
  beq $v0, $at, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Greater - S32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a > 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  slti $at, $v0, 43
  bne $at, $zero, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Less - S32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a < 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  slti $at, $v0, 42
  beq $at, $zero, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Greater-Than - S32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a >= 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  slti $at, $v0, 42
  bne $at, $zero, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Const) - Less-Than - S32", "[branchConst]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a;
      if(a <= 42) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  slti $at, $v0, 43
  beq $at, $zero, LABEL_test_if_0001
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

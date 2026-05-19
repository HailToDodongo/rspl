#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Branch (Var vs. Var) - Equal - U32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a,b;
      if(a == b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bne $v0, $v1, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Var) - Not-Equal - U32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a,b;
      if(a != b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  beq $v0, $v1, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Var) - Greater - U32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a,b;
      if(a > b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  sltu $at, $v1, $v0
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

TEST_CASE("Branch (Var vs. Var) - Less - U32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a,b;
      if(a < b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  sltu $at, $v0, $v1
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

TEST_CASE("Branch (Var vs. Var) - Greater-Than - U32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a,b;
      if(a >= b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  sltu $at, $v0, $v1
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

TEST_CASE("Branch (Var vs. Var) - Less-Than - U32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      u32<$v0> a,b;
      if(a <= b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  sltu $at, $v1, $v0
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

// Signed

TEST_CASE("Branch (Var vs. Var) - Equal - S32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a,b;
      if(a == b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  bne $v0, $v1, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Var) - Not-Equal - S32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a,b;
      if(a != b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  beq $v0, $v1, LABEL_test_if_0001
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

TEST_CASE("Branch (Var vs. Var) - Greater - S32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a,b;
      if(a > b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  slt $at, $v1, $v0
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

TEST_CASE("Branch (Var vs. Var) - Less - S32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a,b;
      if(a < b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  slt $at, $v0, $v1
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

TEST_CASE("Branch (Var vs. Var) - Greater-Than - S32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a,b;
      if(a >= b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  slt $at, $v0, $v1
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

TEST_CASE("Branch (Var vs. Var) - Less-Than - S32", "[branchVar]") {
  auto result = rspl::transpileSource(
      R"(function test_if() {
      s32<$v0> a,b;
      if(a <= b) { a += 1111; } else { a += 2222; }
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test_if:
  slt $at, $v1, $v0
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

#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

// Command args past $a0-$a3 are not provided by the RSPQ dispatcher and must
// be loaded from the command buffer, addressed relative to $gp which points
// past the current command (same math as the load_arg() builtin).

TEST_CASE("Command Args - args past $a0-$a3 are loaded from the buffer",
          "[commandArgs]") {
  auto result = rspl::transpileSource(
      R"(command<0> Cmd_Test(u32 a, u32 b, u32 c, u32 d, u32<$s5> e, u16<$s6> f)
{
  a += e;
  b += f;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(Cmd_Test:
  lw $s5, %lo(RSPQ_DMEM_BUFFER  -8)($gp)
  lhu $s6, %lo(RSPQ_DMEM_BUFFER  -4)($gp)
  addu $a0, $a0, $s5
  addu $a1, $a1, $s6
  j RSPQ_Loop
  nop)");
}

TEST_CASE("Command Args - typed loads and auto-allocated register",
          "[commandArgs]") {
  auto result = rspl::transpileSource(
      R"(command<1> Cmd_Auto(u32 a, u32 b, u32 c, u32 d, u32 e, u8<$t7> f)
{
  a += e;
  b += f;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(Cmd_Auto:
  lw $s7, %lo(RSPQ_DMEM_BUFFER  -8)($gp)
  lbu $t7, %lo(RSPQ_DMEM_BUFFER  -4)($gp)
  addu $a0, $a0, $s7
  addu $a1, $a1, $t7
  j RSPQ_Loop
  nop)");
}

TEST_CASE("Command Args - exactly 4 args emit no loads", "[commandArgs]") {
  auto result = rspl::transpileSource(
      R"(command<2> Cmd_Four(u32 a, u32 b, u32 c, u32 d)
{
  a += d;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(Cmd_Four:
  addu $a0, $a0, $a3
  j RSPQ_Loop
  nop)");
}

TEST_CASE("Command Args - functions never load args from the buffer",
          "[commandArgs]") {
  auto result = rspl::transpileSource(
      R"(function FuncManyArgs(u32 a, u32 b, u32 c, u32 d, u32<$s5> e)
{
  a += e;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(FuncManyArgs:
  addu $a0, $a0, $s5
  jr $ra
  nop)");
}

TEST_CASE("Command Args - offsets scale with argument count",
          "[commandArgs]") {
  // 5 args -> argSize 20, arg 4 sits at 16 - 20 = -4
  auto result = rspl::transpileSource(
      R"(command<3> Cmd_Five(u32 a, u32 b, u32 c, u32 d, u32<$s5> e)
{
  a += e;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(Cmd_Five:
  lw $s5, %lo(RSPQ_DMEM_BUFFER  -4)($gp)
  addu $a0, $a0, $s5
  j RSPQ_Loop
  nop)");
}

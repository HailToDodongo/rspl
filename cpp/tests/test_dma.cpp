#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("DMA - dma_in (sync) with explicit size", "[dma]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32<$s4> dest = 0x1000;
      u32<$s0> rdram = 0x2000;
      u32<$t0> size = 32;
      dma_in(dest, rdram, size);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $s4, $zero, 4096
  addiu $s0, $zero, 8192
  addiu $t0, $zero, 32
  addiu $t0, $t0, -1
  addiu $t2, $zero, 12
  jal DMAExec
  nop
  jr $ra
  nop)");
}

TEST_CASE("DMA - dma_out_async with explicit size", "[dma]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32<$s4> dest = 0x1000;
      u32<$s0> rdram = 0x2000;
      u32<$t0> size = 64;
      dma_out_async(dest, rdram, size);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $s4, $zero, 4096
  addiu $s0, $zero, 8192
  addiu $t0, $zero, 64
  addiu $t0, $t0, -1
  addiu $t2, $zero, -32768
  jal DMAExec
  nop
  jr $ra
  nop)");
}

TEST_CASE("DMA - dma_in_async with memory dest (2-arg)", "[dma]") {
  auto result = rspl::transpileSource(
      R"(state { u8 BUFF[64]; }
function test() {
      u32<$s0> rdram = 0x2000;
      dma_in_async(BUFF, rdram);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $s0, $zero, 8192
  ori $t0, $zero, 63
  ori $s4, $zero, %lo(BUFF)
  or $t2, $zero, $zero
  jal DMAExec
  nop
  jr $ra
  nop)");
}

TEST_CASE("DMA - dma_in_async with register dest (3-arg)", "[dma]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32<$s4> dest = 0x1000;
      u32<$s0> rdram = 0x2000;
      u32<$t0> size = 64;
      dma_in_async(dest, rdram, size);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $s4, $zero, 4096
  addiu $s0, $zero, 8192
  addiu $t0, $zero, 64
  addiu $t0, $t0, -1
  or $t2, $zero, $zero
  jal DMAExec
  nop
  jr $ra
  nop)");
}

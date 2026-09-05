#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

// load_byte_lo/hi and store_byte_lo/hi map to lbv/sbv, which address a byte
// element 0-15: element = lane * 2 + (low ? 1 : 0).

static void requireByteThrowsWith(const char *src, const char *msgPart) {
  REQUIRE_THROWS_AS(rspl::transpileSource(src, {.rspqWrapper = false}),
                    std::runtime_error);
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    INFO(e.what());
    REQUIRE(std::string(e.what()).find(msgPart) != std::string::npos);
  }
}

TEST_CASE("ByteMem - load_byte_lo/hi element from the lane", "[byteMem]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      v.x = load_byte_hi(addr, 0x10);
      v.x = load_byte_lo(addr, 0x10);
      v.z = load_byte_hi(addr);
      v.W = load_byte_lo(addr, -5);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // lane x=0, z=2, W=7 -> even element is the high byte, odd the low byte
  REQUIRE(result.asm_ == R"(test:
  lbv $v01, 0, 16, $t0
  lbv $v01, 1, 16, $t0
  lbv $v01, 4, 0, $t0
  lbv $v01, 15, -5, $t0
  jr $ra
  nop)");
}

TEST_CASE("ByteMem - store_byte_lo/hi element from the lane", "[byteMem]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      store_byte_lo(v.x, addr, 0x10);
      store_byte_hi(v.W, addr, 63);
      store_byte_lo(v.y, addr, -64);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  sbv $v01, 1, 16, $t0
  sbv $v01, 14, 63, $t0
  sbv $v01, 3, -64, $t0
  jr $ra
  nop)");
}

TEST_CASE("ByteMem - memory label address goes through $at", "[byteMem]") {
  auto result = rspl::transpileSource(
      R"(state { u8 BUFF[64]; }
function test() {
      vec16<$v01> v;
      v.y = load_byte_lo(BUFF, 4);
      store_byte_lo(v.y, BUFF);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  ori $at, $zero, %lo(BUFF)
  lbv $v01, 3, 4, $at
  ori $at, $zero, %lo(BUFF)
  sbv $v01, 3, 0, $at
  jr $ra
  nop)");
}

TEST_CASE("ByteMem - errors", "[byteMem]") {
  const char *noSwizzle = R"(function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      v = load_byte_lo(addr);
    })";
  requireByteThrowsWith(noSwizzle, "requires a single-lane swizzle");

  const char *multiLane = R"(function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      v.xy = load_byte_lo(addr);
    })";
  requireByteThrowsWith(multiLane, "requires a single-lane swizzle");

  // the immediate is a signed 7-bit field, unscaled
  const char *offHigh = R"(function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      v.x = load_byte_lo(addr, 64);
    })";
  requireByteThrowsWith(offHigh, "range -64 to 63");

  const char *offLow = R"(function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      store_byte_hi(v.x, addr, -65);
    })";
  requireByteThrowsWith(offLow, "range -64 to 63");

  const char *scalarDst = R"(function test() {
      u32<$t1> q;
      u32<$t0> addr;
      q = load_byte_lo(addr);
    })";
  requireByteThrowsWith(scalarDst, "requires a vector variable");

  const char *scalarVal = R"(function test() {
      u32<$t0> addr;
      store_byte_lo(addr, addr);
    })";
  requireByteThrowsWith(scalarVal, "requires a vector variable");
}

#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

static rspl::TranspileResult optTranspile(const std::string &src) {
  return rspl::transpileSource(src, {.rspqWrapper = false, .optimize = true});
}

TEST_CASE("Optimizer E2E - Merge Sequence - Multiply - Zero fractional", "[optMergeSequence]") {
  auto res = optTranspile(R"(
state { vec16 SCREEN_SCALE_OFFSET; }
function test(u32 dummy)
{
  vec32 screenSize;
  screenSize:sint = load(SCREEN_SCALE_OFFSET);
  screenSize:sfract = 0;
  screenSize >>= 8;
  END:
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)
  lqv $v01, 0, 0, $at
  vmudl $v02, $v00, $v31.e7
  vmadm $v01, $v01, $v31.e7
  vmadn $v02, $v00, $v00
  END:
  jr $ra
  nop)");
}

TEST_CASE("Optimizer E2E - Merge Sequence - Multiply - Non-Zero fractional (no opt)", "[optMergeSequence]") {
  auto res = optTranspile(R"(
state { vec16 SCREEN_SCALE_OFFSET; }
function test(u32 dummy)
{
  vec32 screenSize;
  screenSize:sint = load(SCREEN_SCALE_OFFSET);
  screenSize:sfract = 1;
  screenSize >>= 8;
  END:
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)
  lqv $v01, 0, 0, $at
  vxor $v02, $v00, $v30.e7
  vmudl $v02, $v02, $v31.e7
  vmadm $v01, $v01, $v31.e7
  vmadn $v02, $v00, $v00
  END:
  jr $ra
  nop)");
}

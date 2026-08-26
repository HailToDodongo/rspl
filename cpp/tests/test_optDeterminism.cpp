#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

// The reorder annealer must be byte-reproducible when a fixed seed and a
// fixed iteration count are given (per-variant PRNG streams make the result
// independent of worker-thread scheduling; a pinned worker count keeps the
// batch size stable).

static const char *DET_SRC = R"(state { u32 FOO; vec16 BAR; }
function test()
{
  u32<$t0> a = load(FOO);
  u32<$t1> b = load(FOO);
  vec16<$v01> v1 = load(BAR).xyzwxyzw;
  vec16<$v02> v2 = v1 * v1.x;
  vec16<$v03> v3 = v2 + v1;
  a += 2;
  b += a;
  store(v3, BAR);
  store(a, FOO);
  store(b, FOO, 4);
})";

static std::string runDet(uint32_t seed) {
  auto result = rspl::transpileSource(
      DET_SRC, {.rspqWrapper = false,
                .reorder = true,
                .optWorkers = 2,
                .optSeed = seed,
                .optIters = 4});
  return result.asm_;
}

TEST_CASE("Optimizer - Determinism - same seed, same output",
          "[optDeterminism]") {
  auto a = runDet(1234);
  auto b = runDet(1234);
  REQUIRE(!a.empty());
  REQUIRE(a == b);
}

TEST_CASE("Optimizer - Determinism - repeated runs stay stable",
          "[optDeterminism]") {
  auto first = runDet(99);
  for (int i = 0; i < 2; ++i) {
    REQUIRE(runDet(99) == first);
  }
}

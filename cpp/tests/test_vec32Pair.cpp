#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

// A vec32 always names both of its registers. `vec32<$a, $b>` puts the two
// halves in any two registers; without the second one the register right
// after the first is picked, once, at declaration.

static void requirePairThrowsWith(const char *src, const char *msgPart) {
  REQUIRE_THROWS_AS(rspl::transpileSource(src, {.rspqWrapper = false}),
                    std::runtime_error);
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    INFO(e.what());
    REQUIRE(std::string(e.what()).find(msgPart) != std::string::npos);
  }
}

TEST_CASE("Vec32Pair - arithmetic uses the declared halves", "[vec32Pair]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v05, $v12> tmp;
      vec32<$v20, $v21> adj;
      tmp = adj;
      tmp += adj;
      tmp -= adj;
      tmp *= adj;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v05, $v00, $v20
  vor $v12, $v00, $v21
  vaddc $v12, $v12, $v21.v
  vadd $v05, $v05, $v20.v
  vsubc $v12, $v12, $v21.v
  vsub $v05, $v05, $v20.v
  vmudl $v29, $v12, $v21.v
  vmadm $v29, $v05, $v21.v
  vmadn $v12, $v12, $v20.v
  vmadh $v05, $v05, $v20.v
  jr $ra
  nop)");
}

TEST_CASE("Vec32Pair - casts pick the right half", "[vec32Pair]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v05, $v12> tmp;
      vec16<$v02> a;
      tmp:ufract += a.x;
      tmp:sint += a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // :ufract is the fraction half ($v12), :sint the integer half ($v05)
  REQUIRE(result.asm_ == R"(test:
  vaddc $v12, $v12, $v02.e0
  vadd $v05, $v05, $v02.e0
  jr $ra
  nop)");
}

TEST_CASE("Vec32Pair - builtins use the declared halves", "[vec32Pair]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v05, $v12> tmp;
      vec32<$v20, $v21> adj;
      tmp = get_acc();
      swap(tmp, adj);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vsar $v05, COP2_ACC_HI
  vsar $v12, COP2_ACC_MD
  vxor $v05, $v05, $v20
  vxor $v20, $v05, $v20
  vxor $v05, $v05, $v20
  vxor $v12, $v12, $v21
  vxor $v21, $v12, $v21
  vxor $v12, $v12, $v21
  jr $ra
  nop)");
}

TEST_CASE("Vec32Pair - loads and stores hit both halves", "[vec32Pair]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v05, $v12> tmp;
      u32<$t0> ptr;
      tmp = load(ptr);
      store(tmp, ptr);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  lqv $v05, 0, 0, $t0
  lqv $v12, 0, 16, $t0
  sqv $v05, 0, 0, $t0
  sqv $v12, 0, 16, $t0
  jr $ra
  nop)");
}

TEST_CASE("Vec32Pair - a single register still pairs with the next one",
          "[vec32Pair]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v05> tmp;
      vec16<$v02> a;
      tmp:ufract += a.x;
      tmp:sint += a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vaddc $v06, $v06, $v02.e0
  vadd $v05, $v05, $v02.e0
  jr $ra
  nop)");
}

TEST_CASE("Vec32Pair - both halves are reserved", "[vec32Pair]") {
  requirePairThrowsWith(R"(function test() {
      vec32<$v05, $v12> t;
      vec16<$v12> other;
    })", "already used for variable 't'");

  requirePairThrowsWith(R"(function test() {
      vec16<$v12> other;
      vec32<$v05, $v12> t;
    })", "already used for variable 'other'");

  // undef releases both of them again
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v05, $v12> t;
      undef t;
      vec16<$v12> other;
      vec16<$v05> other2;
    })",
      {.rspqWrapper = false});
  REQUIRE(result.warn.empty());
}

TEST_CASE("Vec32Pair - errors", "[vec32Pair]") {
  requirePairThrowsWith(R"(function test() {
      vec32<$v05, $v05> t;
    })", "two different registers");

  requirePairThrowsWith(R"(function test() {
      vec16<$v05, $v12> t;
    })", "Only vec32 variables can specify two registers");

  requirePairThrowsWith(R"(function test() {
      vec32<$v05, $t0> t;
    })", "is not a vector register");

  requirePairThrowsWith(R"(function test() {
      vec32<$v05, $v12> a, b;
    })", "declares a single variable");
}

// --- alias(): borrow the register another variable already lives in ------

TEST_CASE("Alias - vec32 built from two in-use vec16 registers",
          "[vec32Pair]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v08> fogScaleOffset;
      vec16<$v09> pos;
      vec32<$v13> posClip;
      vec32<alias(fogScaleOffset), alias(pos)> invW;
      invW.w = invert_half(posClip).w;
      invW.W = invert_half(posClip).W;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // only the addressed lane of each host register is written
  REQUIRE(result.asm_ == R"(test:
  vrcph $v08.e3, $v13.e3
  vrcpl $v09.e3, $v14.e3
  vrcph $v08.e3, $v00.e3
  vrcph $v08.e7, $v13.e7
  vrcpl $v09.e7, $v14.e7
  vrcph $v08.e7, $v00.e7
  jr $ra
  nop)");
}

TEST_CASE("Alias - vec16 rename and vec32 half", "[vec32Pair]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v13> big;
      vec16<$v08> foo;
      vec16<alias(big:ufract)> half;
      vec16<alias(foo)> renamed;
      vec16<$v02> a;
      a = half;
      a = renamed;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v02, $v00, $v14
  vor $v02, $v00, $v08
  jr $ra
  nop)");
}

TEST_CASE("Alias - scalars", "[vec32Pair]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32<$t3> ptr;
      u32<alias(ptr)> p2;
      p2 += 1;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $t3, $t3, 1
  jr $ra
  nop)");
}

TEST_CASE("Alias - borrowed registers are not reserved", "[vec32Pair]") {
  // $v08 stays owned by foo, so the owned half ($v09) is the only one the
  // alias claims — declaring another variable on it must still fail.
  requirePairThrowsWith(R"(function test() {
      vec16<$v08> foo;
      vec32<alias(foo), $v09> t;
      vec16<$v09> other;
    })", "already used for variable 't'");
}

TEST_CASE("Alias - undef guard", "[vec32Pair]") {
  requirePairThrowsWith(R"(function test() {
      vec16<$v08> foo;
      vec32<alias(foo), $v09> t;
      undef foo;
    })", "while 't' still aliases one of its registers");

  // dropping the alias first releases the host again
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v08> foo;
      vec32<alias(foo), $v09> t;
      undef t;
      undef foo;
      vec16<$v08> reused;
    })",
      {.rspqWrapper = false});
  REQUIRE(result.warn.empty());
}

TEST_CASE("Alias - errors", "[vec32Pair]") {
  requirePairThrowsWith(R"(function test() {
      vec32<$v13> big;
      vec16<$v08> f;
      vec32<alias(big), alias(f)> t;
    })", "ambiguous for a vec32");

  requirePairThrowsWith(R"(function test() {
      vec16<$v08> foo;
      vec32<alias(foo), alias(foo)> t;
    })", "two different registers");

  requirePairThrowsWith(R"(function test() {
      vec16<alias(nope)> t;
    })", "nope not known");
}

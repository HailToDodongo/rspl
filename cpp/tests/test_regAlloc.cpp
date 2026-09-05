#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

// Auto-allocated variables step around registers that the same block still
// asks for by name further down, so a greedy pick cannot squat on a register
// an explicit declaration needs later.

static std::string asmFor(const char *src) {
  auto r = rspl::transpileSource(src, {.rspqWrapper = false});
  REQUIRE(r.warn.empty());
  return r.asm_;
}

static std::string errorFor(const char *src) {
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    return e.what();
  }
  return {};
}

TEST_CASE("RegAlloc - auto vars avoid a register needed later in the block",
          "[regAlloc]") {
  auto asm_ = asmFor(R"(function callee(u32<$t0> arg);
function test()
{
  u32 counter;
  u32 limit;
  loop {
    u32<$t0> arg;
    arg = counter;
    callee(arg);
    counter += 1;
  } while(counter != limit)
})");

  INFO(asm_);
  // counter/limit must not take $t0, which the loop body needs by name
  REQUIRE(asm_.find("or $t0, $zero, $t1") != std::string::npos);
  REQUIRE(asm_.find("addiu $t1, $t1, 1") != std::string::npos);
}

TEST_CASE("RegAlloc - a sibling block does not reserve anything",
          "[regAlloc]") {
  // the explicit $t0 lives in a later, separate block; `temp` is long dead by
  // then, so it should still get the first free register
  auto asm_ = asmFor(R"(function test()
{
  u32<$t5> c;
  loop {
    u32 temp;
    temp = c;
  } while(c != c)
  loop {
    u32<$t0> arg;
    arg = c;
  } while(c != c)
})");

  INFO(asm_);
  REQUIRE(asm_.find("or $t0, $zero, $t5") != std::string::npos);
}

TEST_CASE("RegAlloc - both halves of a vec32 are kept clear", "[regAlloc]") {
  auto asm_ = asmFor(R"(function test()
{
  u32<$t5> c;
  loop {
    vec32 auto32;
    vec16<$v01> pinned;
    auto32 += auto32;
    pinned += pinned;
  } while(c != c)
})");

  INFO(asm_);
  // $v01 is spoken for, so the pair lands on $v02/$v03
  REQUIRE(asm_.find("vaddc $v03, $v03, $v03.v") != std::string::npos);
  REQUIRE(asm_.find("vadd $v02, $v02, $v02.v") != std::string::npos);
}

TEST_CASE("RegAlloc - falls back when nothing else is free", "[regAlloc]") {
  // every allocatable scalar but $t9 is taken, and $t9 is wanted later:
  // the allocator must still hand it out rather than give up
  std::string src = "function test()\n{\n";
  for (const char *r : {"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8",
                        "k0", "k1", "sp", "fp", "s0", "s1", "s2", "s3", "s4",
                        "s5", "s6", "s7"}) {
    src += "  u32<$" + std::string(r) + "> p_" + r + ";\n";
  }
  src += "  u32 autoVar;\n  autoVar = p_t0;\n";
  src += "  u32<$t9> late;\n  late = p_t0;\n}\n";

  auto err = errorFor(src.c_str());
  INFO(err);
  // the clash is reported, not an early "out of registers"
  REQUIRE(err.find("already used for variable 'autoVar'") != std::string::npos);
  REQUIRE(err.find("Out of free registers") == std::string::npos);
}

TEST_CASE("RegAlloc - running out of registers says so", "[regAlloc]") {
  // 24 vector registers live here, and the four still free are all pinned by
  // name further down: `extra` is one over what the register file holds.
  auto err = errorFor(R"(command<0> test(u32 a)
{
  vec32 m0, m1, m2, m3;
  vec16 n0, n1, n2;
  vec16 nm, ns;
  vec16 gb;
  vec32 scr;
  vec16<$v12> screenOffset;
  vec16<$v10> normScaleW;
  vec16<$v09> uvGenArgs;
  vec16<$v08> pos;
  vec16<$v07> norm;
  vec32<$v05> posClip;
  vec16<$v04> color;
  vec16 extra;
  {
    vec16<$v02> lightDirVec;
    vec16<$v01> lightDirScale;
    vec16<$v03> uv;
    vec16<$v11> oldNorm;
    oldNorm += lightDirVec;
    uv += lightDirScale;
  }
})");

  INFO(err);
  REQUIRE(err.find("already used for variable 'extra'") != std::string::npos);
  // ...and explains that this is pressure, not a naming mistake
  REQUIRE(err.find("out of vector registers") != std::string::npos);
}

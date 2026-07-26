#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "pipeline.h"

#include <string>

// Function patching: only the named functions are optimized and spliced into
// an already-emitted .S file, leaving every other byte of it untouched.

static const std::string OLD_ASM =
    "## header\n"
    ".text\n"
    "funcA:\n"
    "  old a1\n"
    "  old a2\n"
    "funcB:\n"
    "  old b1\n"
    "funcC:\n"
    "  old c1\n"
    "OVERLAY_CODE_END:\n";

static const std::string NEW_ASM =
    "## header\n"
    ".text\n"
    "funcA:\n"
    "  new a1\n"
    "funcB:\n"
    "  new b1\n"
    "  new b2\n"
    "  new b3\n"
    "funcC:\n"
    "  new c1\n"
    "OVERLAY_CODE_END:\n";

TEST_CASE("Patch - getFunctionStartEnd covers label and body", "[patch]") {
  auto [start, end] = rspl::getFunctionStartEnd(OLD_ASM, "funcA");
  REQUIRE(OLD_ASM.substr(start, end - start) ==
          "funcA:\n"
          "  old a1\n"
          "  old a2");
}

TEST_CASE("Patch - getFunctionStartEnd on last function", "[patch]") {
  auto [start, end] = rspl::getFunctionStartEnd(OLD_ASM, "funcC");
  REQUIRE(OLD_ASM.substr(start, end - start) ==
          "funcC:\n"
          "  old c1");
}

TEST_CASE("Patch - unknown function throws", "[patch]") {
  REQUIRE_THROWS_WITH(
      rspl::getFunctionStartEnd(OLD_ASM, "nope"),
      Catch::Matchers::ContainsSubstring(
          "Function nope not found in output file!"));
}

TEST_CASE("Patch - single function replaced, rest untouched", "[patch]") {
  auto out = rspl::patchAsmFunctions(OLD_ASM, NEW_ASM, {"funcB"});
  REQUIRE(out ==
          "## header\n"
          ".text\n"
          "funcA:\n"
          "  old a1\n"
          "  old a2\n"
          "funcB:\n"
          "  new b1\n"
          "  new b2\n"
          "  new b3\n"
          "funcC:\n"
          "  old c1\n"
          "OVERLAY_CODE_END:\n");
}

// Patching several functions must stay correct even though earlier splices
// shift the offsets of later ones.
TEST_CASE("Patch - multiple functions of differing length", "[patch]") {
  auto out = rspl::patchAsmFunctions(OLD_ASM, NEW_ASM, {"funcA", "funcB"});
  REQUIRE(out ==
          "## header\n"
          ".text\n"
          "funcA:\n"
          "  new a1\n"
          "funcB:\n"
          "  new b1\n"
          "  new b2\n"
          "  new b3\n"
          "funcC:\n"
          "  old c1\n"
          "OVERLAY_CODE_END:\n");
}

TEST_CASE("Patch - patching every function equals the new listing", "[patch]") {
  auto out =
      rspl::patchAsmFunctions(OLD_ASM, NEW_ASM, {"funcA", "funcB", "funcC"});
  REQUIRE(out == NEW_ASM);
}

TEST_CASE("Patch - empty function list is a no-op", "[patch]") {
  REQUIRE(rspl::patchAsmFunctions(OLD_ASM, NEW_ASM, {}) == OLD_ASM);
}

TEST_CASE("Patch - missing in target file throws", "[patch]") {
  REQUIRE_THROWS_WITH(
      rspl::patchAsmFunctions(OLD_ASM, NEW_ASM, {"funcD"}),
      Catch::Matchers::ContainsSubstring(
          "Function funcD not found in output file!"));
}

// Only the listed functions take part in optimization.
TEST_CASE("Patch - restricts optimization to listed functions", "[patch]") {
  const char *src = R"(function funcA() {
      u32<$t0> a = 1;
      u32<$t1> b = 2;
    }
    function funcB() {
      u32<$t0> a = 1;
      u32<$t1> b = 2;
    }
    function funcTail() {
      u32<$t0> a = 1;
    })";

  rspl::TranspileConfig full;
  full.rspqWrapper = false;
  full.optimize = true;

  rspl::TranspileConfig patched = full;
  patched.patchFunctions = {"funcA"};

  auto asmFull = rspl::transpileSource(src, full).asm_;
  auto asmPatched = rspl::transpileSource(src, patched).asm_;

  // funcA is optimized in both runs, so it comes out identical...
  auto [fs, fe] = rspl::getFunctionStartEnd(asmFull, "funcA");
  auto [ps, pe] = rspl::getFunctionStartEnd(asmPatched, "funcA");
  REQUIRE(asmFull.substr(fs, fe - fs) == asmPatched.substr(ps, pe - ps));

  // ...while funcB is only optimized in the full run (delay slot filled).
  auto [fs2, fe2] = rspl::getFunctionStartEnd(asmFull, "funcB");
  auto [ps2, pe2] = rspl::getFunctionStartEnd(asmPatched, "funcB");
  REQUIRE(asmFull.substr(fs2, fe2 - fs2) != asmPatched.substr(ps2, pe2 - ps2));
}

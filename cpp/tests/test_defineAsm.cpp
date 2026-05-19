#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Define (ASM) - Define in ASM", "[defineAsm]") {
  auto result = rspl::transpileSource(
      R"(
      include "rsp_queue.inc"
      include "rdpq_macros.h"

      #define SOME_DEF_A 1
      #define SOME_DEF_B 2

      state{}

      #define SOME_DEF_C 3

      command<0> test(u32 a)
      {
      }

      #define SOME_DEF_D 4
    )",
      {.rspqWrapper = true});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_.find("#define SOME_DEF_A 1") != std::string::npos);
  REQUIRE(result.asm_.find("#define SOME_DEF_B 2") != std::string::npos);
  REQUIRE(result.asm_.find("#define SOME_DEF_C 3") != std::string::npos);
  REQUIRE(result.asm_.find("#define SOME_DEF_D 4") != std::string::npos);
}

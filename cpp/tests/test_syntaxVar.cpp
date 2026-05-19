#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Syntax - Vars - Invalid type (scalar reg for vec)", "[syntaxVar]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test() {
  u32<$v03> a;
})",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test() {
  u32<$v03> a;
})",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Cannot use vector register for scalar variable") != std::string::npos);
  }
}

TEST_CASE("Syntax - Vars - Invalid type (vec reg for scalar)", "[syntaxVar]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test() {
  vec16<$t0> a;
})",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test() {
  vec16<$t0> a;
})",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Cannot use scalar register for vector variable") != std::string::npos);
  }
}

TEST_CASE("Syntax - Vars - Invalid (swizzle in decl)", "[syntaxVar]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test() {
  vec16<$v03> a.x;
})",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test() {
  vec16<$v03> a.x;
})",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find("Syntax error") !=
            std::string::npos);
  }
}

TEST_CASE("Syntax - Vars - Invalid (cast in decl)", "[syntaxVar]") {
  REQUIRE_THROWS_AS(
      rspl::transpileSource(
          R"(function test() {
  vec16<$v03> a:sint;
})",
          {.rspqWrapper = false}),
      std::runtime_error);
  try {
    rspl::transpileSource(
        R"(function test() {
  vec16<$v03> a:sint;
})",
        {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Variable name cannot contain a cast") != std::string::npos);
  }
}

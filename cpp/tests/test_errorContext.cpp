#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

// Errors quote the offending line plus one either side, numbered the way the
// author sees them (an #include splices whole files in, so the preprocessed
// line number is not the one in the editor).

static std::string errorFor(const char *src) {
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    return e.what();
  }
  return {};
}

TEST_CASE("ErrorContext - quotes the line and its neighbours",
          "[errorContext]") {
  auto err = errorFor(R"(function test()
{
  vec32<$v05> a;
  vec16<$v05> b;
  a += a;
})");

  INFO(err);
  REQUIRE(err.find("already used for variable 'a'") != std::string::npos);
  // the offending line is marked, the neighbours are not
  REQUIRE(err.find(" > 4 |   vec16<$v05> b;") != std::string::npos);
  REQUIRE(err.find("   3 |   vec32<$v05> a;") != std::string::npos);
  REQUIRE(err.find("   5 |   a += a;") != std::string::npos);
}

TEST_CASE("ErrorContext - indentation is preserved", "[errorContext]") {
  auto err = errorFor(R"(function test()
{
  u32<$t1> c;
  loop {
        vec32<$v05> a;
        vec16<$v05> b;
  } while(c != c)
})");

  INFO(err);
  REQUIRE(err.find(" > 6 |         vec16<$v05> b;") != std::string::npos);
  REQUIRE(err.find("   5 |         vec32<$v05> a;") != std::string::npos);
}

TEST_CASE("ErrorContext - clamps at the start of the file",
          "[errorContext]") {
  // an error on the first line must not walk off the front
  auto err = errorFor(R"(u32<$t0> dup;
u32<$t0> dup2;
function test() { })");

  INFO(err);
  REQUIRE(!err.empty());
  REQUIRE(err.find(" > 2 | u32<$t0> dup2;") != std::string::npos);
}

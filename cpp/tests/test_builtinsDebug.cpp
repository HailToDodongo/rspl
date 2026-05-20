#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "pipeline.h"

#include <string>

// --- set_rsp_status ---

TEST_CASE("Builtins - Debug - set_rsp_status() - scalar", "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32<$t0> a;
      set_rsp_status(a);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  mtc0 $t0, COP0_SP_STATUS
  jr $ra
  nop)");
}

TEST_CASE("Builtins - Debug - set_rsp_status() - scalar literal",
          "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      set_rsp_status(42);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $at, $zero, 42
  mtc0 $at, COP0_SP_STATUS
  jr $ra
  nop)");
}

TEST_CASE("Builtins - Debug - set_rsp_status() - fails with no argument",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      set_rsp_status();
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("requires 1 scalar"));
}

TEST_CASE("Builtins - Debug - set_rsp_status() - fails with left side",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      u32<$t0> a = set_rsp_status();
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("must not have a left side"));
}

TEST_CASE("Builtins - Debug - set_rsp_status() - fails with vector",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      vec16<$v01> a;
      set_rsp_status(a);
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("scalar argument"));
}

// --- print ---

TEST_CASE("Builtins - Debug - print() - scalar", "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32<$t0> a;
      u32<$t1> b;
      print(a, b);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  .set macro # print
  emux_dump_gpr $t0, $t1
  .set noat # print
  .set nomacro # print
  jr $ra
  nop)");
}

TEST_CASE("Builtins - Debug - print() - vector", "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a;
      vec16<$v03> b;
      print(a, b);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  .set macro # print
  emux_dump_vpr $v01, $v03
  .set noat # print
  .set nomacro # print
  jr $ra
  nop)");
}

TEST_CASE("Builtins - Debug - print() - string", "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      print("hello", "world");
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  .set macro # print
  emux_log_string "hello", "world"
  .set noat # print
  .set nomacro # print
  jr $ra
  nop)");
}

TEST_CASE("Builtins - Debug - print() - fails with no arguments",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      print();
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("requires at least one argument"));
}

TEST_CASE("Builtins - Debug - print() - fails with left side",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      u32<$t0> a = print();
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("cannot have a left side"));
}

TEST_CASE("Builtins - Debug - print() - fails with mixed types",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      u32<$t0> a;
      print(a, "hello");
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("same type"));
}

TEST_CASE("Builtins - Debug - print() - fails with number literal",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      print(42);
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("variables or strings"));
}

TEST_CASE("Builtins - Debug - print() - fails with mixed scalar/vector",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      u32<$t0> a;
      vec16<$v01> b;
      print(a, b);
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("mixed scalar/vector"));
}

// --- printf ---

TEST_CASE("Builtins - Debug - printf() - basic scalar", "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32<$t0> a;
      printf("hello %d", a);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  .set macro # print
  emux_printf "hello %dt0"
  .set noat # print
  .set nomacro # print
  jr $ra
  nop)");
}

TEST_CASE("Builtins - Debug - printf() - vec32 with swizzle",
          "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> a;
      printf("result %f", a.x);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_.find("emux_printf") != std::string::npos);
  REQUIRE(result.asm_.find("result %f") != std::string::npos);
}

TEST_CASE("Builtins - Debug - printf() - vec16", "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v03> a;
      printf("val %d", a);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_.find("emux_printf") != std::string::npos);
  REQUIRE(result.asm_.find("val %d") != std::string::npos);
}

TEST_CASE("Builtins - Debug - printf() - string only", "[builtinsDebug]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      printf("hello world");
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_.find("emux_printf") != std::string::npos);
  REQUIRE(result.asm_.find("hello world") != std::string::npos);
}

TEST_CASE("Builtins - Debug - printf() - fails with no arguments",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      printf();
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("requires at least one argument"));
}

TEST_CASE("Builtins - Debug - printf() - fails with left side",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      u32<$t0> a = printf("hello");
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("cannot have a left side"));
}

TEST_CASE("Builtins - Debug - printf() - fails with non-string first arg",
          "[builtinsDebug]") {
  REQUIRE_THROWS_WITH(
      rspl::transpileSource(
          R"(function test() {
      u32<$t0> a;
      printf(a);
    })",
          {.rspqWrapper = false}),
      Catch::Matchers::ContainsSubstring("first argument to be a string"));
}
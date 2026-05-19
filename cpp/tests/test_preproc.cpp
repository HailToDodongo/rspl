#include <catch2/catch_test_macros.hpp>
#include "preproc.h"

#include <string>
#include <unordered_map>

static std::string preproc(const std::string &src) {
  std::unordered_map<std::string, rspl::DefineEntry> defines;
  return rspl::preprocFull(src, defines, ".");
}

TEST_CASE("Preproc - Define - Basic", "[preproc]") {
  auto src = R"(
      #define TEST 42
      macro test() {
        u32 x = TEST;
      }
    )";
  auto res = preproc(src);
  REQUIRE(res.find("u32 x = 42;") != std::string::npos);
}

TEST_CASE("Preproc - Define - Multiple", "[preproc]") {
  auto src = R"(
      #define TEST 42
      #define TEST_AB 43

      macro test() {
        u32 x = TEST;
        u32 y = TEST_AB;
      }
    )";
  auto res = preproc(src);
  REQUIRE(res.find("u32 x = 42;") != std::string::npos);
  REQUIRE(res.find("u32 y = 43;") != std::string::npos);
}

TEST_CASE("Preproc - Define - Deps", "[preproc]") {
  auto src = R"(
      #define TEST 42
      #define TEST_AB TEST+1

      macro test() {
        u32 x = TEST;
        u32 y = TEST_AB;
      }
    )";
  auto res = preproc(src);
  REQUIRE(res.find("u32 x = 42;") != std::string::npos);
  REQUIRE(res.find("u32 y = 42+1;") != std::string::npos);
}

TEST_CASE("Preproc - Define - Partial", "[preproc]") {
  auto src = R"(
      #define my 42

      macro my_function() {
        u32 x = my;
      }
    )";
  auto res = preproc(src);
  REQUIRE(res.find("u32 x = 42;") != std::string::npos);
}

TEST_CASE("Preproc - Define - Undef", "[preproc]") {
  auto src = R"(
      #define TEST 42
      macro test() {
        u32 x = TEST;
      }
      #undef TEST
    )";
  auto res = preproc(src);
  REQUIRE(res.find("u32 x = 42;") != std::string::npos);
}

TEST_CASE("Preproc - Define - Undef Before usage", "[preproc]") {
  auto src = R"(
      #define TEST 42
      #undef TEST

      macro test() {
        u32 x = TEST;
      }
    )";
  auto res = preproc(src);
  REQUIRE(res.find("u32 x = TEST;") != std::string::npos);
}

TEST_CASE("Preproc - Define - Empty", "[preproc]") {
  auto src = R"(
      #define
      macro test() {
        u32 x = TEST;
      }
    )";
  REQUIRE_THROWS_AS(preproc(src), std::runtime_error);
  try {
    preproc(src);
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Invalid #define statement") != std::string::npos);
  }
}

TEST_CASE("Preproc - Ifdef - Basic", "[preproc]") {
  auto src = R"(
      #define TEST 42

      #ifdef TEST2
        macro test2() {}
      #endif

      #ifdef TEST
        macro test() {}
      #endif
    )";
  auto res = preproc(src);
  REQUIRE(res.find("macro test() {}") != std::string::npos);
  REQUIRE(res.find("macro test2() {}") == std::string::npos);
}

TEST_CASE("Preproc - Ifdef - Else", "[preproc]") {
  auto src = R"(
      #define TEST 42

      #ifdef TEST2
        macro test2() {}
      #else
        macro test() {}
      #endif
    )";
  auto res = preproc(src);
  REQUIRE(res.find("macro test() {}") != std::string::npos);
  REQUIRE(res.find("macro test2() {}") == std::string::npos);
}

TEST_CASE("Preproc - Ifdef - define (true)", "[preproc]") {
  auto src = R"(
      #define TEST 42

      #ifdef TEST
        #define VAL 1
      #else
        #define VAL 2
      #endif
      VAL
    )";
  auto res = preproc(src);
  // After preprocessing, VAL should be 1
  REQUIRE(res.find("1") != std::string::npos);
}

TEST_CASE("Preproc - Ifdef - define (false)", "[preproc]") {
  auto src = R"(
      #define TEST 42

      #ifdef TEST_OTHER
        #define VAL 1
      #else
        #define VAL 2
      #endif
      VAL
    )";
  auto res = preproc(src);
  REQUIRE(res.find("2") != std::string::npos);
}

TEST_CASE("Preproc - Ifdef - nested", "[preproc]") {
  auto src = R"(
      #ifdef TEST
        #ifdef TEST2
        #endif
      #endif

    )";
  REQUIRE_THROWS_AS(preproc(src), std::runtime_error);
  try {
    preproc(src);
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find(
        "Nested #ifdef") != std::string::npos);
  }
}

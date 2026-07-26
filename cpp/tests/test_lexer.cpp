#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include "parser/lexer.h"

#include <string>
#include <vector>

using namespace rspl::parser;

// All expectations in this file were verified against the actual moo lexer
// (grammar.cjs) via the differential dump harness. Some look odd on purpose:
// moo picks the FIRST matching rule in declaration order, not the longest
// match, so keyword literals can split identifiers.

static std::vector<std::pair<Tok, std::string>>
lex(const std::string &src) {
  std::vector<std::pair<Tok, std::string>> out;
  for (const auto &t : tokenize(src)) {
    if (t.type == Tok::End) break;
    out.push_back({t.type, t.value});
  }
  return out;
}

TEST_CASE("Lexer - keyword-prefix identifiers split", "[lexer]") {
  // "iffy" is NOT one identifier: the "if" literal wins first
  REQUIRE(lex("iffy") == std::vector<std::pair<Tok, std::string>>{
      {Tok::KWIf, "if"}, {Tok::VarName, "fy"}});
  REQUIRE(lex("elsewhere") == std::vector<std::pair<Tok, std::string>>{
      {Tok::KWElse, "else"}, {Tok::VarName, "where"}});
  REQUIRE(lex("u32abc") == std::vector<std::pair<Tok, std::string>>{
      {Tok::DataType, "u32"}, {Tok::VarName, "abc"}});
}

TEST_CASE("Lexer - cast suffix is part of VarName", "[lexer]") {
  REQUIRE(lex("a:ufract") == std::vector<std::pair<Tok, std::string>>{
      {Tok::VarName, "a:ufract"}});
  // uppercase after ':' is not a cast — three tokens
  REQUIRE(lex("a:UFract") == std::vector<std::pair<Tok, std::string>>{
      {Tok::VarName, "a"}, {Tok::Colon, ":"}, {Tok::VarName, "UFract"}});
  // label form: bare ':' stays separate
  REQUIRE(lex("foo:") == std::vector<std::pair<Tok, std::string>>{
      {Tok::VarName, "foo"}, {Tok::Colon, ":"}});
}

TEST_CASE("Lexer - minus binds to number literals", "[lexer]") {
  // value rules come before operator rules
  REQUIRE(lex("a -1") == std::vector<std::pair<Tok, std::string>>{
      {Tok::VarName, "a"}, {Tok::ValueDec, "-1"}});
  REQUIRE(lex("a - 1") == std::vector<std::pair<Tok, std::string>>{
      {Tok::VarName, "a"}, {Tok::OperatorLR, "-"}, {Tok::ValueDec, "1"}});
  REQUIRE(lex("a- 1") == std::vector<std::pair<Tok, std::string>>{
      {Tok::VarName, "a"}, {Tok::OperatorLR, "-"}, {Tok::ValueDec, "1"}});
  REQUIRE(lex("-2.75") == std::vector<std::pair<Tok, std::string>>{
      {Tok::ValueFloat, "-2.75"}});
}

TEST_CASE("Lexer - number formats and separators", "[lexer]") {
  REQUIRE(lex("0xFF' 0b1'0 1'000") ==
          std::vector<std::pair<Tok, std::string>>{
              {Tok::ValueHex, "0xFF'"},
              {Tok::ValueBin, "0b1'0"},
              {Tok::ValueDec, "1'000"}});
  Token hexTok{Tok::ValueHex, "0x1'0", 1, 1, false};
  REQUIRE(parseNumericToken(hexTok) == 16.0);
  Token decTok{Tok::ValueDec, "-1'2", 1, 1, false};
  REQUIRE(parseNumericToken(decTok) == -12.0);
}

TEST_CASE("Lexer - operator precedence by rule order", "[lexer]") {
  REQUIRE(lex(">>>= >>> >>= >> +*= +* ~| ~ !=") ==
          std::vector<std::pair<Tok, std::string>>{
              {Tok::OperatorSelfR, ">>>="},
              {Tok::OperatorLR, ">>>"},
              {Tok::OperatorSelfR, ">>="},
              {Tok::OperatorLR, ">>"},
              {Tok::OperatorSelfR, "+*="},
              {Tok::OperatorLR, "+*"},
              {Tok::OperatorLR, "~|"},
              {Tok::OperatorUnary, "~"},
              {Tok::OperatorCompare, "!="}});
  // '<' and '>' are TypeStart/TypeEnd, not operators
  REQUIRE(lex("< > <= >=") == std::vector<std::pair<Tok, std::string>>{
      {Tok::TypeStart, "<"}, {Tok::TypeEnd, ">"},
      {Tok::OperatorCompare, "<="}, {Tok::OperatorCompare, ">="}});
}

TEST_CASE("Lexer - registers longest-first within rule", "[lexer]") {
  REQUIRE(lex("$v0 $v00 $zero $t9") ==
          std::vector<std::pair<Tok, std::string>>{
              {Tok::Register, "$v0"}, {Tok::Register, "$v00"},
              {Tok::Register, "$zero"}, {Tok::Register, "$t9"}});
  // unknown register suffix splits
  REQUIRE(lex("$zerox") == std::vector<std::pair<Tok, std::string>>{
      {Tok::Register, "$zero"}, {Tok::VarName, "x"}});
}

TEST_CASE("Lexer - swizzles are normalized", "[lexer]") {
  REQUIRE(lex(".0123 .xyzwxyzw .7 .xy") ==
          std::vector<std::pair<Tok, std::string>>{
              {Tok::Swizzle, "xyzw"}, {Tok::Swizzle, "xyzwxyzw"},
              {Tok::Swizzle, "W"}, {Tok::Swizzle, "xy"}});
}

TEST_CASE("Lexer - strings and line/col tracking", "[lexer]") {
  auto toks = tokenize("a\n  \"x y\"");
  REQUIRE(toks[0].line == 1);
  REQUIRE(toks[0].col == 1);
  REQUIRE(toks[1].type == Tok::String);
  REQUIRE(toks[1].value == "\"x y\"");
  REQUIRE(toks[1].line == 2);
  REQUIRE(toks[1].col == 3);
  REQUIRE(toks[1].spaceBefore);
}

TEST_CASE("Lexer - invalid input throws with position", "[lexer]") {
  REQUIRE_THROWS_WITH(tokenize("a # b"),
                      "invalid syntax at line 1 col 3");
  // "1." cannot lex: dec then a bare dot with no valid swizzle
  REQUIRE_THROWS_WITH(tokenize("x 1."),
                      "invalid syntax at line 1 col 4");
}

#include "lexer.h"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace rspl::parser {

// Faithful port of the moo lexer from src/lib/grammar.ne. moo compiles the
// rule list into one big alternation tried in declaration order: the FIRST
// rule that matches at the current position wins (not the longest match).
// Within a single rule, literal alternatives are sorted longest-first.
// This ordering is observable (e.g. "iffy" lexes as KWIf + VarName "fy")
// and must not be "improved" here — the JS grammar behaves the same way.

namespace {

struct LitRule {
  Tok type;
  std::vector<std::string> lits; // sorted longest-first
};

// Sorts longest-first, keeping declaration order for equal lengths
// (JS Array.sort is stable).
std::vector<std::string> byLength(std::vector<std::string> v) {
  std::stable_sort(v.begin(), v.end(),
                   [](const std::string &a, const std::string &b) {
                     return a.size() > b.size();
                   });
  return v;
}

const std::vector<LitRule> &litRulesPre() {
  // Rules between String and the value regexes, in grammar order.
  static const std::vector<LitRule> rules = {
      {Tok::DataType,
       byLength({"u8", "s8", "u16", "s16", "u32", "s32", "vec32", "vec16"})},
      {Tok::Register, byLength({
           "$at", "$zero", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
           "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
           "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
           "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
           "$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07",
           "$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15",
           "$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23",
           "$v24", "$v25", "$v26", "$v27", "$v28", "$v29", "$v30", "$v31"})},
      {Tok::Swizzle, byLength({
           ".xxzzXXZZ", ".yywwYYWW",
           ".xxxxXXXX", ".yyyyYYYY", ".zzzzZZZZ", ".wwwwWWWW",
           ".xxxxxxxx", ".yyyyyyyy", ".zzzzzzzz", ".wwwwwwww",
           ".XXXXXXXX", ".YYYYYYYY", ".ZZZZZZZZ", ".WWWWWWWW",
           ".xyzwxyzw", ".xyzw", ".XYZW",
           ".xy", ".zw", ".XY", ".ZW",
           ".x", ".y", ".z", ".w", ".X", ".Y", ".Z", ".W",
           ".00224466", ".11335577",
           ".00004444", ".11115555", ".22226666", ".33337777",
           ".00000000", ".11111111", ".22222222", ".33333333",
           ".44444444", ".55555555", ".66666666", ".77777777",
           ".01230123", ".0123", ".4567",
           ".01", ".23", ".45", ".67",
           ".0", ".1", ".2", ".3", ".4", ".5", ".6", ".7"})},
      {Tok::FunctionType, byLength({"function", "command", "macro", "shader"})},
      {Tok::KWIf, {"if"}},
      {Tok::KWLoop, {"loop"}},
      {Tok::KWElse, {"else"}},
      {Tok::KWBreak, {"break"}},
      {Tok::KWWhile, {"while"}},
      {Tok::KWGoto, {"goto"}},
      {Tok::KWExtern, {"extern"}},
      {Tok::KWContinue, {"continue"}},
      {Tok::KWInclude, {"include"}},
      {Tok::KWConst, {"const"}},
      {Tok::KWUndef, {"undef"}},
      {Tok::KWExit, {"exit"}},
      {Tok::KWAlign, {"alignas"}},
      {Tok::KWUniform, {"uniform"}},
      {Tok::KWAttr, {"attribute"}},
  };
  return rules;
}

const std::vector<LitRule> &litRulesPost() {
  // Rules between the value regexes and VarName, in grammar order.
  static const std::vector<LitRule> rules = {
      {Tok::OperatorSelfR, byLength({
           "&&=", "||=", "&=", "|=", "^=", ">>>=", "<<=", ">>=", "+*=",
           "+=", "-=", "*=", "/="})},
      {Tok::OperatorCompare, byLength({"<=", ">=", "==", "!="})},
      {Tok::OperatorLR, byLength({
           "&&", "||", ">>>", ">>", "<<", "+*",
           "+", "-", "*", "/", "&", "~|", "|", "^"})},
      {Tok::OperatorUnary, {"!", "~"}},
      {Tok::QuestionMark, {"?"}},
      {Tok::BlockStart, {"{"}},
      {Tok::BlockEnd, {"}"}},
      {Tok::ArgsStart, {"("}},
      {Tok::ArgsEnd, {")"}},
      {Tok::TypeStart, {"<"}},
      {Tok::TypeEnd, {">"}},
      {Tok::StmEnd, {";"}},
      {Tok::Seperator, {","}},
      {Tok::IdxStart, {"["}},
      {Tok::IdxEnd, {"]"}},
      {Tok::AnnoStart, {"@"}},
      {Tok::Assignment, {"="}},
  };
  return rules;
}

inline bool isDigit(char c) { return c >= '0' && c <= '9'; }
inline bool isHexDigit(char c) {
  return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
inline bool isVarChar(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || isDigit(c) ||
         c == '_';
}
inline bool isLowerAlnum(char c) {
  return (c >= 'a' && c <= 'z') || isDigit(c);
}

// Digit-lane swizzle aliases ("0"->"x" ... "7"->"W"), from NORM_SWIZZLE.
char normSwizzleChar(char c) {
  static const char *MAP = "xyzwXYZW";
  if (c >= '0' && c <= '7') return MAP[c - '0'];
  return c;
}

size_t matchLit(const std::string &src, size_t pos, const LitRule &rule) {
  for (const auto &lit : rule.lits) {
    if (src.compare(pos, lit.size(), lit) == 0) return lit.size();
  }
  return 0;
}

// /".*?"/ — non-greedy, '.' does not match newline
size_t matchString(const std::string &src, size_t pos) {
  if (src[pos] != '"') return 0;
  for (size_t i = pos + 1; i < src.size(); ++i) {
    if (src[i] == '\n') return 0;
    if (src[i] == '"') return i - pos + 1;
  }
  return 0;
}

// /0x[0-9A-Fa-f']+/
size_t matchHex(const std::string &src, size_t pos) {
  if (src.compare(pos, 2, "0x") != 0) return 0;
  size_t i = pos + 2;
  while (i < src.size() && (isHexDigit(src[i]) || src[i] == '\'')) ++i;
  return (i > pos + 2) ? i - pos : 0;
}

// /0b[0-1']+/
size_t matchBin(const std::string &src, size_t pos) {
  if (src.compare(pos, 2, "0b") != 0) return 0;
  size_t i = pos + 2;
  while (i < src.size() &&
         (src[i] == '0' || src[i] == '1' || src[i] == '\'')) ++i;
  return (i > pos + 2) ? i - pos : 0;
}

// /[-]?[0-9]+[.][0-9]+/
size_t matchFloat(const std::string &src, size_t pos) {
  size_t i = pos;
  if (i < src.size() && src[i] == '-') ++i;
  size_t intStart = i;
  while (i < src.size() && isDigit(src[i])) ++i;
  if (i == intStart) return 0;
  if (i >= src.size() || src[i] != '.') return 0;
  ++i;
  size_t fracStart = i;
  while (i < src.size() && isDigit(src[i])) ++i;
  if (i == fracStart) return 0;
  return i - pos;
}

// /[-]?[0-9][0-9']*/ — note: the minus sign is part of the token
size_t matchDec(const std::string &src, size_t pos) {
  size_t i = pos;
  if (i < src.size() && src[i] == '-') ++i;
  if (i >= src.size() || !isDigit(src[i])) return 0;
  ++i;
  while (i < src.size() && (isDigit(src[i]) || src[i] == '\'')) ++i;
  return i - pos;
}

// /[a-zA-Z0-9_]+(?:\:[a-z0-9]+)?/ — the cast suffix is part of the name
size_t matchVarName(const std::string &src, size_t pos) {
  size_t i = pos;
  while (i < src.size() && isVarChar(src[i])) ++i;
  if (i == pos) return 0;
  if (i < src.size() && src[i] == ':') {
    size_t j = i + 1;
    while (j < src.size() && isLowerAlnum(src[j])) ++j;
    if (j > i + 1) i = j; // suffix needs at least one [a-z0-9]
  }
  return i - pos;
}

} // namespace

const char *tokName(Tok t) {
  switch (t) {
  case Tok::End: return "end of input";
  case Tok::String: return "String";
  case Tok::DataType: return "DataType";
  case Tok::Register: return "Registers";
  case Tok::Swizzle: return "Swizzle";
  case Tok::FunctionType: return "FunctionType";
  case Tok::KWIf: return "if";
  case Tok::KWLoop: return "loop";
  case Tok::KWElse: return "else";
  case Tok::KWBreak: return "break";
  case Tok::KWWhile: return "while";
  case Tok::KWGoto: return "goto";
  case Tok::KWExtern: return "extern";
  case Tok::KWContinue: return "continue";
  case Tok::KWInclude: return "include";
  case Tok::KWConst: return "const";
  case Tok::KWUndef: return "undef";
  case Tok::KWExit: return "exit";
  case Tok::KWAlign: return "alignas";
  case Tok::KWUniform: return "uniform";
  case Tok::KWAttr: return "attribute";
  case Tok::ValueHex: return "ValueHex";
  case Tok::ValueBin: return "ValueBin";
  case Tok::ValueFloat: return "ValueFloat";
  case Tok::ValueDec: return "ValueDec";
  case Tok::OperatorSelfR: return "OperatorSelfR";
  case Tok::OperatorCompare: return "OperatorCompare";
  case Tok::OperatorLR: return "OperatorLR";
  case Tok::OperatorUnary: return "OperatorUnary";
  case Tok::QuestionMark: return "'?'";
  case Tok::BlockStart: return "'{'";
  case Tok::BlockEnd: return "'}'";
  case Tok::ArgsStart: return "'('";
  case Tok::ArgsEnd: return "')'";
  case Tok::TypeStart: return "'<'";
  case Tok::TypeEnd: return "'>'";
  case Tok::StmEnd: return "';'";
  case Tok::Seperator: return "','";
  case Tok::IdxStart: return "'['";
  case Tok::IdxEnd: return "']'";
  case Tok::AnnoStart: return "'@'";
  case Tok::Assignment: return "'='";
  case Tok::VarName: return "VarName";
  case Tok::Colon: return "':'";
  }
  return "?";
}

std::vector<Token> tokenize(const std::string &src) {
  std::vector<Token> out;
  size_t pos = 0;
  uint32_t line = 1, col = 1;
  bool spaceBefore = false;

  auto push = [&](Tok type, size_t len, std::string value = {}) {
    Token t;
    t.type = type;
    t.value = value.empty() ? src.substr(pos, len) : std::move(value);
    t.line = line;
    t.col = col;
    t.spaceBefore = spaceBefore;
    out.push_back(std::move(t));
    spaceBefore = false;
    pos += len;
    col += static_cast<uint32_t>(len); // no matched token contains newlines
  };

  while (pos < src.size()) {
    char c = src[pos];

    // whitespace rule /[ \t\n]+/ (last in grammar order, but nothing else
    // matches these characters, so checking it early is equivalent)
    if (c == ' ' || c == '\t' || c == '\n') {
      size_t i = pos;
      while (i < src.size() &&
             (src[i] == ' ' || src[i] == '\t' || src[i] == '\n')) {
        if (src[i] == '\n') { ++line; col = 1; } else { ++col; }
        ++i;
      }
      pos = i;
      spaceBefore = true;
      continue;
    }

    size_t len;
    if ((len = matchString(src, pos))) { push(Tok::String, len); continue; }

    bool matched = false;
    for (const auto &rule : litRulesPre()) {
      if ((len = matchLit(src, pos, rule))) {
        if (rule.type == Tok::Swizzle) {
          std::string norm;
          for (size_t i = 1; i < len; ++i) // strip leading '.'
            norm += normSwizzleChar(src[pos + i]);
          push(Tok::Swizzle, len, norm);
        } else {
          push(rule.type, len);
        }
        matched = true;
        break;
      }
    }
    if (matched) continue;

    if ((len = matchHex(src, pos)))   { push(Tok::ValueHex, len); continue; }
    if ((len = matchBin(src, pos)))   { push(Tok::ValueBin, len); continue; }
    if ((len = matchFloat(src, pos))) { push(Tok::ValueFloat, len); continue; }
    if ((len = matchDec(src, pos)))   { push(Tok::ValueDec, len); continue; }

    for (const auto &rule : litRulesPost()) {
      if ((len = matchLit(src, pos, rule))) {
        push(rule.type, len);
        matched = true;
        break;
      }
    }
    if (matched) continue;

    if ((len = matchVarName(src, pos))) { push(Tok::VarName, len); continue; }
    if (c == ':') { push(Tok::Colon, 1); continue; }

    throw std::runtime_error("invalid syntax at line " + std::to_string(line) +
                             " col " + std::to_string(col));
  }

  Token end;
  end.type = Tok::End;
  end.line = line;
  end.col = col;
  end.spaceBefore = spaceBefore;
  out.push_back(std::move(end));
  return out;
}

double parseNumericToken(const Token &tok) {
  std::string s;
  s.reserve(tok.value.size());
  for (char c : tok.value) {
    if (c != '\'') s += c;
  }
  switch (tok.type) {
  case Tok::ValueHex:
    return static_cast<double>(std::strtoll(s.c_str() + 2, nullptr, 16));
  case Tok::ValueBin:
    return static_cast<double>(std::strtoll(s.c_str() + 2, nullptr, 2));
  case Tok::ValueFloat:
    return std::strtod(s.c_str(), nullptr);
  case Tok::ValueDec:
    return static_cast<double>(std::strtoll(s.c_str(), nullptr, 10));
  default:
    throw std::runtime_error("parseNumericToken: not a numeric token");
  }
}

} // namespace rspl::parser

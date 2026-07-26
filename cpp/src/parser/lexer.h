#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rspl::parser {

// Token types, mirroring the moo lexer in src/lib/grammar.ne.
enum class Tok : uint8_t {
  End = 0,
  String,
  DataType,
  Register,
  Swizzle,
  FunctionType,
  KWIf, KWLoop, KWElse, KWBreak, KWWhile, KWGoto, KWExtern, KWContinue,
  KWInclude, KWConst, KWUndef, KWExit, KWAlign, KWUniform, KWAttr,
  ValueHex, ValueBin, ValueFloat, ValueDec,
  OperatorSelfR, OperatorCompare, OperatorLR, OperatorUnary,
  QuestionMark,
  BlockStart, BlockEnd,
  ArgsStart, ArgsEnd,
  TypeStart, TypeEnd,
  StmEnd, Seperator,
  IdxStart, IdxEnd,
  AnnoStart,
  Assignment,
  VarName,
  Colon,
};

const char *tokName(Tok t);

struct Token {
  Tok type = Tok::End;
  std::string value;        // token text; swizzles are normalized (".0" -> "x")
  uint32_t line = 1;        // 1-based, like moo
  uint32_t col = 1;         // 1-based
  bool spaceBefore = false; // whitespace directly before this token
};

/// Tokenizes preprocessed RSPL source. Whitespace is folded into the
/// following token's `spaceBefore` flag; the stream ends with a Tok::End.
/// Throws std::runtime_error ("invalid syntax at line N col M") on
/// unmatchable input, matching the moo lexer.
std::vector<Token> tokenize(const std::string &src);

/// Parses a numeric token (ValueHex/Bin/Float/Dec) exactly like the grammar:
/// "'" separators stripped, hex/bin prefixes handled, decimals may be
/// negative, floats parsed as-is.
double parseNumericToken(const Token &tok);

} // namespace rspl::parser

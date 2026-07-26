// Dev tool: dump lexer tokens in the same format as the moo reference
// dumper (scratch moodump.mjs), for differential testing of the lexer.
#include "parser/lexer.h"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace rspl::parser;

// moo rule names, for diffing against the JS lexer output
static const char *mooName(Tok t) {
  switch (t) {
  case Tok::String: return "String";
  case Tok::DataType: return "DataType";
  case Tok::Register: return "Registers";
  case Tok::Swizzle: return "Swizzle";
  case Tok::FunctionType: return "FunctionType";
  case Tok::KWIf: return "KWIf";
  case Tok::KWLoop: return "KWLoop";
  case Tok::KWElse: return "KWElse";
  case Tok::KWBreak: return "KWBreak";
  case Tok::KWWhile: return "KWWhile";
  case Tok::KWGoto: return "KWGoto";
  case Tok::KWExtern: return "KWExtern";
  case Tok::KWContinue: return "KWContinue";
  case Tok::KWInclude: return "KWInclude";
  case Tok::KWConst: return "KWConst";
  case Tok::KWUndef: return "KWUndef";
  case Tok::KWExit: return "KWExit";
  case Tok::KWAlign: return "KWAlign";
  case Tok::KWUniform: return "KWUniform";
  case Tok::KWAttr: return "KWAttr";
  case Tok::ValueHex: return "ValueHex";
  case Tok::ValueBin: return "ValueBin";
  case Tok::ValueFloat: return "ValueFloat";
  case Tok::ValueDec: return "ValueDec";
  case Tok::OperatorSelfR: return "OperatorSelfR";
  case Tok::OperatorCompare: return "OperatorCompare";
  case Tok::OperatorLR: return "OperatorLR";
  case Tok::OperatorUnary: return "OperatorUnary";
  case Tok::QuestionMark: return "QuestionMark";
  case Tok::BlockStart: return "BlockStart";
  case Tok::BlockEnd: return "BlockEnd";
  case Tok::ArgsStart: return "ArgsStart";
  case Tok::ArgsEnd: return "ArgsEnd";
  case Tok::TypeStart: return "TypeStart";
  case Tok::TypeEnd: return "TypeEnd";
  case Tok::StmEnd: return "StmEnd";
  case Tok::Seperator: return "Seperator";
  case Tok::IdxStart: return "IdxStart";
  case Tok::IdxEnd: return "IdxEnd";
  case Tok::AnnoStart: return "AnnoStart";
  case Tok::Assignment: return "Assignment";
  case Tok::VarName: return "VarName";
  case Tok::Colon: return "Colon";
  case Tok::End: return "End";
  }
  return "?";
}

int main(int argc, char **argv) {
  if (argc < 2) { std::cerr << "usage: lexdump <file>\n"; return 1; }
  std::ifstream f(argv[1]);
  if (!f) { std::cerr << "cannot open " << argv[1] << "\n"; return 1; }
  std::ostringstream ss;
  ss << f.rdbuf();

  try {
    auto tokens = tokenize(ss.str());
    for (const auto &tok : tokens) {
      if (tok.type == Tok::End) break;
      std::cout << mooName(tok.type) << "|" << tok.value << "|" << tok.line
                << "|" << tok.col << "|" << (tok.spaceBefore ? 1 : 0) << "\n";
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}

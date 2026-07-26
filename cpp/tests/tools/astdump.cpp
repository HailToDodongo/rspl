// Dev tool: dump the canonical AST JSON for a preprocessed RSPL file.
//   astdump native <file>  — native C++ parser
//   astdump js <file>      — parse.js -> ast::parseJson (oracle path)
// Both go through the same serializer, so byte-equal output means the
// native parser produces an identical AST.
#include "ast.h"
#include "ast_json.h"
#include "parser/parser.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

static std::string runJsParser(const std::string &path) {
  std::string cmd = "node scripts/parse.js --preprocessed \"" + path + "\"";
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) throw std::runtime_error("cannot start JS parser");
  std::string result;
  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) result += buf;
  if (pclose(pipe) != 0) throw std::runtime_error("JS parser failed:\n" + result);
  return result;
}

int main(int argc, char **argv) {
  if (argc < 3) { std::cerr << "usage: astdump native|js <file>\n"; return 1; }
  std::string mode = argv[1];
  try {
    rspl::ast::Program prog;
    if (mode == "native") {
      std::ifstream f(argv[2]);
      if (!f) { std::cerr << "cannot open " << argv[2] << "\n"; return 1; }
      std::ostringstream ss;
      ss << f.rdbuf();
      prog = rspl::parser::parseProgram(ss.str());
    } else {
      prog = rspl::ast::parseJson(runJsParser(argv[2]));
    }
    std::cout << rspl::astToJson(prog, true) << "\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}

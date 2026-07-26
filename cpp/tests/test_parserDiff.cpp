#include <catch2/catch_test_macros.hpp>
#include "ast.h"
#include "ast_json.h"
#include "parser/parser.h"
#include "preproc.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Differential oracle: the native parser must produce byte-identical ASTs
// to the JS parser (scripts/parse.js). Both sides are serialized through
// astToJson and compared as strings. Requires node; skipped when absent.

namespace {

bool nodeAvailable() {
  return std::system("node --version > /dev/null 2>&1") == 0;
}

std::string readFile(const std::string &path) {
  std::ifstream f(path);
  REQUIRE(f.is_open());
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// JS-parser path: preprocessed source -> parse.js -> ast::parseJson
std::string jsAstJson(const std::string &preprocessed) {
  std::string tmpPath = "/tmp/rspl_parser_diff.rspl";
  {
    std::ofstream f(tmpPath);
    REQUIRE(f.is_open());
    f << preprocessed;
  }
  std::string cmd = "node scripts/parse.js --preprocessed " + tmpPath;
  FILE *pipe = popen(cmd.c_str(), "r");
  REQUIRE(pipe != nullptr);
  std::string result;
  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) result += buf;
  REQUIRE(pclose(pipe) == 0);
  auto prog = rspl::ast::parseJson(result);
  return rspl::astToJson(prog);
}

std::string nativeAstJson(const std::string &preprocessed) {
  auto prog = rspl::parser::parseProgram(preprocessed);
  return rspl::astToJson(prog);
}

void requireSameAst(const std::string &preprocessed) {
  REQUIRE(nativeAstJson(preprocessed) == jsAstJson(preprocessed));
}

} // namespace

TEST_CASE("ParserDiff - corpus files", "[parserDiff]") {
  if (!nodeAvailable()) SKIP("node not available");

  // All standalone-parseable .rspl files in the repo (tpxLoops.rspl is a
  // template with unexpanded ${} defines and cannot parse on its own).
  const std::vector<std::string> corpus = {
      "src/tests/examples/3d.rspl",
      "src/tests/examples/mandelbrot.rspl",
      "src/tests/examples/rsp_fx.rspl",
      "src/tests/examples/rsp_mgfx.rspl",
      "src/tests/examples/squares2d.rspl",
      "src/tests/examples/t3d/rsp_tiny3d.rspl",
      "src/tests/examples/t3d/rsp_tinypx.rspl",
  };

  for (const auto &path : corpus) {
    INFO("file: " << path);
    auto slash = path.rfind('/');
    std::string dir = path.substr(0, slash);
    std::unordered_map<std::string, rspl::DefineEntry> defines;
    std::string pre = rspl::preprocFull(readFile(path), defines, dir);
    requireSameAst(pre);
  }
}

TEST_CASE("ParserDiff - statement and calc constructs", "[parserDiff]") {
  if (!nodeAvailable()) SKIP("node not available");

  const std::vector<std::string> snippets = {
      // declarations, self-ops, swizzled assignment
      R"(function f() {
  u32<$t0> a = 1;
  const u32 b = 0x10;
  u32 c, d;
  vec16<$v01> v;
  undef c;
  a = 1;
  a += b;
  a +*= b;
  v.xy = a;
  exit;
})",
      // calc variants: num / var / unary / multi with groups
      R"(function f() {
  x = a;
  x = !a;
  x = ~a;
  x = a.x;
  x = 5;
  x = -3;
  x = 1.5;
  x = 2 + a;
  x = a.x + b.y - 3 * c;
  x = ((a + b) * c) + d;
  x = (a + (b - c)) + d;
})",
      // compares, ternaries, calc func calls
      R"(function f() {
  x = a < b;
  x = a > 5;
  x = a == b.x;
  x = a != 3 ? c : d;
  x = a >= b ? c : 2;
  x = foo();
  x = foo(a, 1, "str");
  x = load(p, 0x10).xyzwxyzw;
})",
      // control flow
      R"(function f() {
  label:
  goto label;
  if(a == b) x = 1;
  if(a < b) { x = 1; } else { y = 2; }
  if(!a) { } else if(b) x = 2; else { z = 3; }
  while(a < b) { break; }
  loop { continue; }
  loop { x = 1; } while(i < 10)
  print(a, "text", 5);
})",
      // sections, annotations, commands
      R"(include "rsp_queue.inc"
state {
  u32 A;
  extern u32 B;
  alignas(16) u16 C[4];
  u8 D[2][8];
  s16 E[3] = {1, 2, -3};
}
@Align(8)
@NoReturn
function decl();
@Relative
function f() {
  f();
}
command<4> cmd(u32 a, u32<$t2> b, vec16 v) {
}
include "post.inc")",
      // magma constructs
      R"(uniform<0> U0 { u32 A; }
uniform U1 {
  extern u32 B;
  vec16 C[2];
}
attribute<0> s16 POS[3];
attribute u32 COL?;
shader s() {
  u32<$t0> vtx;
  @AttrLoader("POS") u32<$t1> p = load(vtx);
  @AttrPatch("POS:nop") p = 1;
  @Anno(10) p = 3;
})",
      // nested scoped blocks
      R"(function f() {
  {
    u32 scoped;
    { u32 nested; }
  }
  x = 1;
})",
  };

  for (size_t i = 0; i < snippets.size(); ++i) {
    INFO("snippet " << i);
    requireSameAst(snippets[i]);
  }
}

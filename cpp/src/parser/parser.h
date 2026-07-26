#pragma once

#include "../ast.h"

#include <string>

namespace rspl::parser {

/// Parses preprocessed RSPL source into an AST program, replacing the
/// JS nearley parser (scripts/parse.js). Behavior mirrors the grammar in
/// src/lib/grammar.ne, including its whitespace-adjacency rules.
/// Throws std::runtime_error ("Syntax error at line N col M: ...") on
/// parse errors and ("invalid syntax at line N col M") on lexer errors.
ast::Program parseProgram(const std::string &source);

} // namespace rspl::parser

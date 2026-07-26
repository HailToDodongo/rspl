#pragma once

#include "ast.h"

#include <string>

namespace rspl {

/// Serializes a parsed program to a canonical JSON string. Used by
/// --ast-dump and by the parser differential tests: both the native parser
/// and the JS-parser path (parse.js JSON -> ast::parseJson) are serialized
/// through this and compared as strings.
std::string astToJson(const ast::Program &prog, bool pretty = false);

} // namespace rspl

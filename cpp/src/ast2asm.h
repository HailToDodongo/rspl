#pragma once

#include "asm.h"
#include "ast.h"

#include <vector>

namespace rspl {

/// Convert a parsed AST program into per-function ASM.
std::vector<AsmFunc> ast2asm(const ast::Program &prog);

} // namespace rspl

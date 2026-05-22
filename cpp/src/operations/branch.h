#pragma once

#include "../asm.h"
#include "../ast.h"

#include <string>
#include <vector>

namespace rspl::ops {

Opcode invertBranchOp(Opcode op);

std::vector<AsmInst> opBranch(const ast::CompareExpr &compare,
                              const std::string &labelElse,
                              bool invert = false);

} // namespace rspl::ops

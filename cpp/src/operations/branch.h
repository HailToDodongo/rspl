#pragma once

#include "../asm.h"
#include "../ast.h"

#include <string>
#include <vector>

namespace rspl::ops {

std::string invertBranchOp(const std::string &op);

std::vector<AsmInst> opBranch(const ast::CompareExpr &compare,
                              const std::string &labelElse,
                              bool invert = false);

} // namespace rspl::ops

#pragma once

#include "../asm.h"
#include "../ast.h"

#include <string>
#include <vector>

namespace rspl::ops {

std::vector<AsmInst> callUserFunction(const std::string &name,
                                      const std::vector<ast::FuncArg> &args);

} // namespace rspl::ops

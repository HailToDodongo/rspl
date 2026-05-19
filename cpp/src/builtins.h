#pragma once

#include "asm.h"
#include "state.h"

#include <functional>
#include <string>
#include <vector>

namespace rspl::builtins {

using BuiltinFn =
    std::function<std::vector<AsmInst>(const VarDef *, // varRes (null if no left side)
                                       const std::vector<ast::FuncArg> &, // args
                                       const std::string & // swizzle
                                       )>;

// Look up a builtin by name. Returns nullptr if not found.
const BuiltinFn *lookup(const std::string &name);

} // namespace rspl::builtins

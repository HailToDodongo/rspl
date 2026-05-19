#pragma once

namespace rspl {

struct AsmFunc;

/// Estimate the cycle cost of a function. Used for optimizer comparison.
int evalFunctionCost(AsmFunc &func);

} // namespace rspl

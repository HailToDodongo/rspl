#pragma once

#include <cstdint>
#include <vector>

namespace rspl {

struct AsmFunc;

// Both functions are thin wrappers over the compact scheduler
// (compact_sched.cpp), which holds the one implementation of the cost model.

/// Search objective: path-aware, integer-scaled cost (hot path x64 with
/// loop multipliers, skipped arms x16, code after the exit x1). Sets
/// func.hotCycles and per-op debug cycles along the walked regions.
int evalFunctionCost(AsmFunc &func);

/// Plain linear walk over every instruction. Used only for the debug
/// annotations written to the output.
int evalFunctionCostLinear(AsmFunc &func);

} // namespace rspl

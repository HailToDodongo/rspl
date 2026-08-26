#pragma once

namespace rspl {

struct AsmFunc;

/// Anneal objective: path-aware, integer-scaled cost (hot path x64 with
/// loop multipliers, skipped arms x16, code after the exit x1). Sets
/// func.hotCycles and per-op debug cycles along the walked regions.
int evalFunctionCost(AsmFunc &func);

/// Plain linear walk over every instruction (legacy / JS-compatible
/// numbering). Used only for the debug annotations written to the output.
int evalFunctionCostLinear(AsmFunc &func);

} // namespace rspl

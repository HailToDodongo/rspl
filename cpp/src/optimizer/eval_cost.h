#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace rspl {

struct AsmFunc;

/// Anneal objective: path-aware, integer-scaled cost (hot path x64 with
/// loop multipliers, skipped arms x16, code after the exit x1). Sets
/// func.hotCycles and per-op debug cycles along the walked regions.
int evalFunctionCost(AsmFunc &func);

/// Plain linear walk over every instruction (legacy / JS-compatible
/// numbering). Used only for the debug annotations written to the output.
int evalFunctionCostLinear(AsmFunc &func);

/// Hot path of a function as seen by the objective: the fall-through spine
/// (see evalFunctionCost). Indices refer to the OP-only list (labels
/// skipped) in asm_ order; `asmIndex` maps them back to func.asm_.
struct HotPath {
  std::vector<int> ops;        // op indices along the walk, in issue order
  std::vector<int> asmIndex;   // op index -> index into func.asm_
  std::vector<int> labelBefore;// op index -> index of the label AsmInst
                               // directly preceding it in asm_, or -1
  int opCount = 0;             // total number of ops in the function
  int terminal = 0;            // first op index after the function exit
  std::vector<std::pair<int, int>> loops; // [from, to] op ranges of loops
  std::vector<uint8_t> visited;// per op: on the hot path
};
HotPath evalCollectHotPath(AsmFunc &func);

} // namespace rspl

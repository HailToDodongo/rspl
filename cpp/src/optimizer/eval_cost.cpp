#include "eval_cost.h"
#include "../asm.h"
#include "compact_sched.h"

namespace rspl {

// Copy the per-position cycles of the last evaluation onto the instructions
// (the initial compact order has one entry per asm_ item, in order).
static void writeCycles(const CompactFunc &cf, const CompactState &st,
                        AsmFunc &func) {
  for (size_t k = 0; k < st.seq.size(); ++k)
    if (cf.ops[st.seq[k]].isOp) func.asm_[k].debug.cycle = st.cycle[k];
}

int evalFunctionCost(AsmFunc &func) {
  CompactFunc cf = compactBuild(func);
  CompactState st = compactInitialState(cf);
  int cost = compactEvalCost(cf, st);
  writeCycles(cf, st, func);
  func.hotCycles = st.hotCycles;
  return cost;
}

int evalFunctionCostLinear(AsmFunc &func) {
  CompactFunc cf = compactBuild(func);
  CompactState st = compactInitialState(cf);
  int cycles = compactEvalCostLinear(cf, st);
  writeCycles(cf, st, func);
  return cycles;
}

} // namespace rspl

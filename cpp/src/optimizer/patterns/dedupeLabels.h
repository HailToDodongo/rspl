#pragma once
#include "optimizer/asm_optimizer.h"
#include <string>

namespace rspl {

// Two adjacent labels name the same address. Only a compiler-generated one
// may be dropped (its references move to the neighbour); user labels stay,
// they may be referenced from outside this function.
inline void dedupeLabels(AsmFunc &func) {
  for (size_t i = 0; i + 1 < func.asm_.size(); ++i) {
    auto &a = func.asm_[i];
    auto &b = func.asm_[i + 1];
    // Skip __-prefixed labels — these are compiler-internal and should
    // never be deduplicated (matching JS dedupeLabels.js:22).
    if (a.type != AsmType::LABEL || b.type != AsmType::LABEL) continue;
    if (a.cold->label.starts_with("__") || b.cold->label.starts_with("__")) continue;
    size_t drop;
    if (isGeneratedLabel(func, a.cold->label)) drop = i;
    else if (isGeneratedLabel(func, b.cold->label)) drop = i + 1;
    else continue;
    std::string from = func.asm_[drop].cold->label;
    std::string to = func.asm_[drop == i ? i + 1 : i].cold->label;
    for (auto &inst : func.asm_) {
      if (inst.cold->labelEnd == from) inst.cold->labelEnd = to;
      for (auto &arg : inst.args) {
        if (arg == from) arg = to;
      }
    }
    func.asm_.erase(func.asm_.begin() + drop);
    --i;
  }
}

} // namespace rspl

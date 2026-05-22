#pragma once
#include "optimizer/asm_optimizer.h"
#include <string>

namespace rspl {

inline void dedupeLabels(AsmFunc &func) {
  for (size_t i = 0; i + 1 < func.asm_.size(); ++i) {
    auto &a = func.asm_[i];
    auto &b = func.asm_[i + 1];
    // Skip __-prefixed labels — these are compiler-internal and should
    // never be deduplicated (matching JS dedupeLabels.js:22).
    if (a.type != AsmType::LABEL || b.type != AsmType::LABEL) continue;
    if (a.cold->label.starts_with("__") || b.cold->label.starts_with("__")) continue;
    std::string from = a.cold->label;
    std::string to = b.cold->label;
    for (auto &inst : func.asm_) {
      if (inst.cold->labelEnd == from) inst.cold->labelEnd = to;
      for (auto &arg : inst.args) {
        if (arg == from) arg = to;
      }
    }
    func.asm_.erase(func.asm_.begin() + i);
    --i;
  }
}

} // namespace rspl

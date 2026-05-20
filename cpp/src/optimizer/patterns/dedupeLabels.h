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
    if (a.label.starts_with("__") || b.label.starts_with("__")) continue;
    std::string from = a.label;
    std::string to = b.label;
    for (auto &inst : func.asm_) {
      if (inst.labelEnd == from) inst.labelEnd = to;
      for (auto &arg : inst.args) {
        if (arg == from) arg = to;
      }
    }
    func.asm_.erase(func.asm_.begin() + i);
    --i;
  }
}

} // namespace rspl

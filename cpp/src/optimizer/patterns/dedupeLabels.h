#pragma once
#include "optimizer/asm_optimizer.h"
#include <string>

namespace rspl {

inline void dedupeLabels(AsmFunc &func) {
  for (size_t i = 0; i + 1 < func.asm_.size(); ++i) {
    if (func.asm_[i].type == AsmType::LABEL &&
        func.asm_[i + 1].type == AsmType::LABEL) {
      std::string from = func.asm_[i].label;
      std::string to = func.asm_[i + 1].label;
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
}

} // namespace rspl

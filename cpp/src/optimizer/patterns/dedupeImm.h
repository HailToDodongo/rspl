#pragma once
#include "optimizer/asm_optimizer.h"

namespace rspl {

inline void dedupeImmediate(AsmFunc &func) {
  for (size_t i = 0; i + 1 < func.asm_.size(); ++i) {
    if (func.asm_[i].op == "or" && func.asm_[i].args.size() == 3 &&
        func.asm_[i].args[1] == "$zero" &&
        func.asm_[i].args[2] == "$zero") {
      std::string reg = func.asm_[i].args[0];
      if (func.asm_[i + 1].op == "addiu" &&
          func.asm_[i + 1].args.size() >= 2 &&
          func.asm_[i + 1].args[0] == reg &&
          func.asm_[i + 1].args[1] == "$zero") {
        func.asm_.erase(func.asm_.begin() + i);
        --i;
      }
    }
  }
}

} // namespace rspl

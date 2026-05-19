#pragma once
#include "optimizer/asm_optimizer.h"

namespace rspl {

inline void tailCall(AsmFunc &func) {
  for (size_t i = 0; i + 3 < func.asm_.size(); ++i) {
    if (func.asm_[i].op == "jal" && func.asm_[i + 1].op == "nop" &&
        func.asm_[i + 2].op == "jr" &&
        func.asm_[i + 2].args.size() >= 1 &&
        func.asm_[i + 2].args[0] == "$ra" &&
        func.asm_[i + 3].op == "nop") {
      func.asm_[i].op = "j";
      func.asm_.erase(func.asm_.begin() + i + 2,
                      func.asm_.begin() + i + 4);
    }
  }
}

} // namespace rspl

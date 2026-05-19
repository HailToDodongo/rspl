#pragma once
#include "optimizer/asm_optimizer.h"

namespace rspl {

inline void removeDeadCode(AsmFunc &func) {
  if (func.asm_.empty()) return;
  int lastSafeIndex = -1;
  for (int i = static_cast<int>(func.asm_.size()) - 1 - 2; i >= 0; --i) {
    const auto &inst = func.asm_[i];
    if (inst.op == "j" || inst.op == "jr") {
      lastSafeIndex = i;
      break;
    }
    if (inst.opFlags & OpFlag::OP_FLAG_IS_NOP) continue;
    break;
  }
  if (lastSafeIndex < 0) return;
  func.asm_.erase(func.asm_.begin() + lastSafeIndex + 2, func.asm_.end());
}

} // namespace rspl

#pragma once
#include "optimizer/asm_optimizer.h"
#include <string>
#include <utility>
#include <vector>

namespace rspl {

inline void dedupeJumps(AsmFunc &func) {
  std::vector<std::pair<std::string, std::string>> labelReplace;
  for (size_t i = 0; i < func.asm_.size(); ++i) {
    if (func.asm_[i].type == AsmType::LABEL) {
      if (i + 1 < func.asm_.size() && func.asm_[i + 1].op == Op::J()) {
        labelReplace.push_back(
            {func.asm_[i].cold->label, func.asm_[i + 1].args[0]});
        if (i >= 2 && func.asm_[i - 2].op == Op::J() &&
            func.asm_[i - 1].type == AsmType::OP &&
            func.asm_[i - 1].op == Op::NOP()) {
          func.asm_.erase(func.asm_.begin() + i,
                          func.asm_.begin() + i + 3);
          --i;
        }
      }
    }
  }

  for (auto &inst : func.asm_) {
    if ((inst.opFlags & OpFlag::OP_FLAG_IS_BRANCH) || inst.op == Op::J() ||
        inst.op == Op::JAL()) {
      if (inst.args.empty()) continue;
      std::string &label = inst.args.back();
      for (const auto &[oldL, newL] : labelReplace) {
        if (label == oldL) label = newL;
      }
      if (inst.cold->labelEnd.empty()) continue;
      for (const auto &[oldL, newL] : labelReplace) {
        if (inst.cold->labelEnd == oldL) inst.cold->labelEnd = newL;
      }
    }
  }
}

} // namespace rspl

#pragma once
#include "asm.h"
#include "optimizer/asm_optimizer.h"
#include "operations/branch.h"
#include "registers.h"
#include <string>

namespace rspl {

inline void branchJump(AsmFunc &func) {
  for (size_t i = 0; i + 4 < func.asm_.size(); ++i) {
    auto &b = func.asm_[i];
    if (!(b.opFlags & OpFlag::OP_FLAG_IS_BRANCH) || b.cold->labelEnd.empty())
      continue;
    if (b.op == Op::J() || b.op == Op::JAL()) continue;
    if (func.asm_[i + 1].op != Op::NOP()) continue;
    Opcode jumpOp = func.asm_[i + 2].op;
    if (jumpOp != Op::J() && jumpOp != Op::JAL()) continue;
    std::string realTarget = func.asm_[i + 2].args[0];
    if (func.asm_[i + 3].op != Op::NOP()) continue;
    if (func.asm_[i + 4].type != AsmType::LABEL) continue;
    if (func.asm_[i + 4].cold->label != b.cold->labelEnd) continue;

    std::string tempLabel = b.cold->labelEnd;
    bool labelUsed = false;
    for (const auto &inst : func.asm_) {
      if (&inst == &b) continue;
      if (inst.cold->labelEnd == tempLabel) { labelUsed = true; break; }
      for (const auto &arg : inst.args)
        if (arg == tempLabel) { labelUsed = true; break; }
      if (labelUsed) break;
    }

    b.cold->labelEnd = realTarget;
    b.args.back() = realTarget;
    Opcode newOp = ops::invertBranchOp(b.op);
    b.op = newOp;

    if (jumpOp == Op::JAL()) {
      func.asm_.erase(func.asm_.begin() + i + 2,
                      func.asm_.begin() + i + 4);
      func.asm_.insert(func.asm_.begin() + static_cast<long>(i),
                       asmOp("ori",
                             {reg::Reg::RA, reg::Reg::ZERO, tempLabel}));
    } else if (labelUsed) {
      func.asm_.erase(func.asm_.begin() + i + 2,
                      func.asm_.begin() + i + 4);
    } else {
      func.asm_.erase(func.asm_.begin() + i + 2,
                      func.asm_.begin() + i + 5);
    }
  }
}

} // namespace rspl

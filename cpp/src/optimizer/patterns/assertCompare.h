#pragma once
#include "asm.h"
#include "optimizer/asm_optimizer.h"
#include "operations/branch.h"
#include "registers.h"
#include <string>

namespace rspl {

inline void assertCompare(AsmFunc &func) {
  static const std::string LABEL_ASSERT = "assertion_failed";
  for (size_t i = 0; i + 5 < func.asm_.size(); ++i) {
    auto &b = func.asm_[i];
    if (!(b.opFlags & OpFlag::OP_FLAG_IS_BRANCH) || b.op.empty() ||
        b.op[0] != 'b')
      continue;
    if (!(func.asm_[i + 1].opFlags & OpFlag::OP_FLAG_IS_NOP)) continue;
    if (func.asm_[i + 2].op != "lui" ||
        func.asm_[i + 2].args[0] != reg::Reg::AT)
      continue;
    if (func.asm_[i + 3].op != "j" ||
        func.asm_[i + 3].args[0] != LABEL_ASSERT)
      continue;
    if (!(func.asm_[i + 4].opFlags & OpFlag::OP_FLAG_IS_NOP)) continue;
    if (func.asm_[i + 5].type != AsmType::LABEL) continue;
    if (func.asm_[i + 5].cold->label != b.args.back()) continue;

    b.op = ops::invertBranchOp(b.op);
    b.args.back() = LABEL_ASSERT;
    b.cold->labelEnd = LABEL_ASSERT;

    AsmInst luiOp = std::move(func.asm_[i + 2]);
    func.asm_.insert(func.asm_.begin() + static_cast<long>(i) + 1,
                     std::move(luiOp));
    func.asm_.erase(func.asm_.begin() + static_cast<long>(i) + 2,
                    func.asm_.begin() + static_cast<long>(i) + 6);
    i += 2;
  }
}

} // namespace rspl

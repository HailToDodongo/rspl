#pragma once
#include "optimizer/asm_optimizer.h"

namespace rspl {

inline void commandAlias(AsmFunc &func) {
  if (func.asm_.size() < 2 || func.type != FuncType::Command) return;

  auto &inst = func.asm_;
  Opcode op0 = inst[0].op;
  bool isBranch =
      (op0 == Op::J() || op0 == Op::JR() || op0 == Op::BEQ() || op0 == Op::BNE());
  if (isBranch && (inst[1].opFlags & OpFlag::OP_FLAG_IS_NOP)) {
    if (!inst[0].args.empty()) {
      func.nameOverride = inst[0].args[0];
    }
    if (inst.size() == 2) {
      inst.clear();
    }
  }
}

} // namespace rspl

#pragma once
#include "optimizer/asm_optimizer.h"
#include "registers.h"
#include <string>

namespace rspl {

inline void mergeSequence(AsmFunc &func) {
  for (size_t i = 0; i + 1 < func.asm_.size(); ++i) {
    auto &a = func.asm_[i];
    auto &b = func.asm_[i + 1];

    // Merge: addiu $x, $zero, N -> addu $y, $x, $z  into addiu $y, $z, N
    if (a.op == Op::ADDIU() && a.args.size() >= 3 && a.args[1] == "$zero" &&
        b.op == Op::ADDU() && b.args.size() >= 3 && b.args[1] == a.args[0]) {
      b.op = Op::ADDIU();
      b.args[1] = b.args[2];
      b.args[2] = a.args[2];
      func.asm_.erase(func.asm_.begin() + i);
      --i;
      continue;
    }

    // Merge consecutive sqrt/reciprocal: first step against VZERO can be
    // combined with the start of the next sequence (JS mergeSequence.js:23-41)
    if (i + 2 < func.asm_.size() &&
        ((a.op == Op::VRSQH() || a.op == Op::VRCPH()) &&
         a.args.size() >= 2 &&
         a.args[1].starts_with(reg::Reg::VZERO) &&
         func.asm_[i + 1].op == a.op &&
         func.asm_[i + 2].op ==
             (a.op == Op::VRSQH() ? Op::VRSQL() : Op::VRCPL()))) {
      func.asm_[i + 1].args[0] = a.args[0];
      func.asm_.erase(func.asm_.begin() + i);
      --i;
      continue;
    }

    // Indirect multiply by zero.
    // vxor $reg, $v00, $v00.?? -> ... -> vmudl $reg, $reg, ...
    // Replace $reg source in vmudl with $v00 and remove the vxor.
    if (a.op == Op::VXOR() && a.args.size() >= 3 &&
        a.args[1] == reg::Reg::VZERO &&
        a.args[2].starts_with(reg::Reg::VZERO)) {
      std::string targetReg = a.args[0];
      // Search forward for a matching vmudl (within 5 instructions)
      for (size_t j = i + 1; j < func.asm_.size() && j <= i + 5; ++j) {
        auto &vmudl = func.asm_[j];
        if (vmudl.op == Op::VMUDL() && vmudl.args.size() >= 2 &&
            vmudl.args[0] == targetReg && vmudl.args[1] == targetReg) {
          vmudl.args[1] = reg::Reg::VZERO;
          func.asm_.erase(func.asm_.begin() + i);
          --i;
          break;
        }
      }
    }
  }
}

} // namespace rspl

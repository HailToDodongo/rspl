#pragma once
#include "optimizer/asm_optimizer.h"

namespace rspl {

inline void tailCall(AsmFunc &func) {
  // Only applies to commands (JS tailCall.js:24). Match JS behaviour:
  // scan for the first jal that can be converted and stop.
  if (func.type != FuncType::Command) return;

  for (size_t i = 0; i + 3 < func.asm_.size(); ++i) {
    bool matched = false;

    // Pattern 1: jal X; nop; jr $ra; nop  →  j X; nop
    if (func.asm_[i].op == "jal" && func.asm_[i + 1].op == "nop" &&
        func.asm_[i + 2].op == "jr" &&
        func.asm_[i + 2].args.size() >= 1 &&
        func.asm_[i + 2].args[0] == "$ra" &&
        func.asm_[i + 3].op == "nop") {
      matched = true;
    }
    // Pattern 2: jal X; nop; j RSPQ_Loop; nop  →  j X; nop
    else if (func.asm_[i].op == "jal" && func.asm_[i + 1].op == "nop" &&
             func.asm_[i + 2].op == "j" &&
             func.asm_[i + 2].args.size() >= 1 &&
             func.asm_[i + 2].args[0] == "RSPQ_Loop" &&
             func.asm_[i + 3].op == "nop") {
      matched = true;
    }

    if (matched) {
      func.asm_[i].op = "j";
      func.asm_.erase(func.asm_.begin() + i + 2,
                      func.asm_.begin() + i + 4);
    }

    // If we found a jal at all (whether converted or not), stop.
    // JS: "if we encounter a jump, but the above condition is not met,
    //      we can stop — otherwise it would mean the return register was changed."
    if (func.asm_[i].op == "j" || func.asm_[i].op == "jal") return;
  }
}

} // namespace rspl

#pragma once
#include "optimizer/asm_optimizer.h"
#include "registers.h"
#include <string>

namespace rspl {

inline void dedupeImmediate(AsmFunc &func) {
  // Ported from JS dedupeImm.js: track the last value written to $at
  // via `ori` and remove redundant `ori $at, $zero, SAME_VALUE`.
  std::string lastAT;
  std::vector<AsmInst> asmNew;
  for (auto &asm_ : func.asm_) {
    bool keep = true;
    if (asm_.type == AsmType::OP) {
      if (asm_.opFlags & OpFlag::OP_FLAG_IS_BRANCH)
        lastAT.clear();

      // Check if this instruction writes to $at
      if (!asm_.args.empty() && asm_.args[0] == reg::Reg::AT) {
        std::string newAT;
        // Only handle "ori" — other writes are assumed to set $at
        // in unknown ways and reset the cache.
        if (asm_.op == "ori" && asm_.args.size() >= 3) {
          newAT = asm_.args[2];
          if (!lastAT.empty() && lastAT == newAT) {
            keep = false; // redundant — same value already in $at
          }
        }
        lastAT = newAT;
      }
    } else {
      lastAT.clear();
    }

    if (keep) asmNew.push_back(std::move(asm_));
  }
  func.asm_ = std::move(asmNew);
}

} // namespace rspl

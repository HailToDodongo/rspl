#include "asm_normalize.h"
#include "asm.h"
#include "registers.h"

#include <unordered_set>

namespace rspl {

// READ_ONLY_OPS: BRANCH_OPS + STORE_OPS + ["mtc0"]
static const std::unordered_set<Opcode> READ_ONLY_OPS = []() {
  std::unordered_set<Opcode> s;
  for (auto *op : {"beq","bne","bgezal","bltzal","bgez","bltz",
       "blez","bgtz","j","jr","jal",
       "sw","sh","sb","sbv","ssv","slv","sdv",
       "sqv","spv","suv","shv","sfv","stv","swv","srv",
       "mtc0"})
    s.insert(getOpcode(op));
  return s;
}();

void normalizeASM(AsmFunc &func) {
  std::vector<AsmInst> result;
  result.reserve(func.asm_.size());

  for (auto &inst : func.asm_) {
    if (inst.type != AsmType::OP ||
        READ_ONLY_OPS.count(inst.op) ||
        inst.args.empty()) {
      result.push_back(std::move(inst));
      continue;
    }

    // Ignore writes to $zero or $vzero (including element-suffixed like $v00.e0)
    std::string targetReg =
        (inst.op == Op::MTC2()) ? inst.args[1] : inst.args[0];
    auto dotPos = targetReg.find('.');
    std::string baseReg =
        (dotPos != std::string::npos) ? targetReg.substr(0, dotPos) : targetReg;
    if (baseReg == reg::Reg::ZERO || baseReg == reg::Reg::VZERO) {
      continue; // drop this instruction
    }

    result.push_back(std::move(inst));
  }

  func.asm_ = std::move(result);
}

} // namespace rspl

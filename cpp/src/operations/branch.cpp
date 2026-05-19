#include "branch.h"
#include "scalar.h"

#include "../asm.h"
#include "../registers.h"
#include "../state.h"
#include "../types.h"

#include <string>
#include <unordered_map>

namespace rspl::ops {

static const std::unordered_map<std::string, std::string> BRANCH_INVERT = {
    {"beq", "bne"},    {"bne", "beq"},     {"bgezal", "bltzal"},
    {"bltzal", "bgezal"}, {"bgez", "bltz"},  {"bltz", "bgez"},
    {"blez", "bgtz"},    {"bgtz", "blez"},
};

static const std::unordered_map<std::string, std::string>
    ZERO_COMB_BRANCH = {
        {"<", "bltz"},
        {"<=", "blez"},
        {">", "bgtz"},
        {">=", "bgez"},
};

std::string invertBranchOp(const std::string &op) {
  auto it = BRANCH_INVERT.find(op);
  if (it == BRANCH_INVERT.end()) {
    state.throwError("Cannot invert branch operation: " + op);
  }
  return it->second;
}

std::vector<AsmInst> opBranch(const ast::CompareExpr &compare,
                              const std::string &labelElse,
                              bool invert) {
  bool isImmediate = (compare.right.type == "num");
  std::string regTestRes;
  if (isImmediate) {
    regTestRes = reg::Reg::AT;
  } else {
    const VarDef *var =
        state.getRequiredVar(compare.right.value, "compare");
    regTestRes = var->reg;
  }

  // Zero-checks can use $zero directly
  if (isImmediate && compare.right.value == "0") {
    isImmediate = false;
    regTestRes = reg::Reg::ZERO;
  }

  VarDef varLeft =
      state.getRequiredVarCopy(compare.left.value, "left");
  std::string regLeft = varLeft.reg;
  std::string baseType = varLeft.type;

  // == and != are simple
  if (compare.op == "==" || compare.op == "!=") {
    std::string opBranch = compare.op == "==" ? "bne" : "beq";
    if (invert) opBranch = invertBranchOp(opBranch);

    std::vector<AsmInst> res;
    if (isImmediate) {
      auto load = loadImmediate(reg::Reg::AT, compare.right.value);
      res.insert(res.end(), load.begin(), load.end());
    }
    res.push_back(
        asmBranch(opBranch, {regLeft, regTestRes, labelElse}, labelElse));
    res.push_back(asmNOP());
    return res;
  }

  // Zero-combination branch ops
  if (!isImmediate && regTestRes == reg::Reg::ZERO) {
    auto it = ZERO_COMB_BRANCH.find(compare.op);
    if (it != ZERO_COMB_BRANCH.end()) {
      std::string op = it->second;
      if (!invert) op = invertBranchOp(op);
      return {asmBranch(op, {regLeft, labelElse}, labelElse), asmNOP()};
    }
  }

  // Transform > and <= into < and >= with swapped args
  std::string op = compare.op;
  std::string valR = isImmediate ? compare.right.value : regTestRes;

  if (op == ">" || op == "<=") {
    if (isImmediate) {
      // Increment immediate to handle the "=" part
      valR = std::to_string(std::stoll(valR) + 1);
      op = op == ">" ? ">=" : "<";
    } else {
      op = op == ">" ? "<" : ">=";
      std::swap(regLeft, regTestRes);
    }
  }

  // If immediate doesn't fit in 16-bit signed range, load it into $at first
  if (isImmediate) {
    int64_t immVal = std::stoll(valR);
    if (immVal < -32768 || immVal > 32767) {
      isImmediate = false;
      regTestRes = reg::Reg::AT;
    }
  }

  // For </>= comparisons, use slt + branch
  std::string opLessThan =
      "slt" + (isImmediate ? std::string("i") : "") +
      (isSigned(baseType) ? "" : "u");

  if (op == "<" || op == ">=") {
    std::string brOp = op == "<" ? "beq" : "bne";
    if (invert) brOp = invertBranchOp(brOp);

    std::vector<AsmInst> res;
    if (!isImmediate && regTestRes == reg::Reg::AT) {
      auto load = loadImmediate(reg::Reg::AT, valR);
      res.insert(res.end(), load.begin(), load.end());
    }
    res.push_back(
        asmOp(opLessThan, {reg::Reg::AT, regLeft,
                            isImmediate ? valR : regTestRes}));
    res.push_back(asmBranch(
        brOp, {reg::Reg::AT, reg::Reg::ZERO, labelElse}, labelElse));
    res.push_back(asmNOP());
    return res;
  }

  state.throwError(
      "Unknown comparison operator: " + compare.op, {});
  return {};
}

} // namespace rspl::ops

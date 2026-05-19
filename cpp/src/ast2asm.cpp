#include "ast2asm.h"

#include "asm.h"
#include "asm_normalize.h"
#include "ast.h"
#include "builtins.h"
#include "operations/branch.h"
#include "operations/scalar.h"
#include "operations/user_function.h"
#include "operations/vector.h"
#include "registers.h"
#include "state.h"
#include "swizzle.h"
#include "types.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

// --- Macro registry ---------------------------------------------------

static std::unordered_map<std::string, const ast::Function *> macros;

// --- Forward declarations ---------------------------------------------

static std::vector<AsmInst>
scopedBlockToAsm(const ast::ScopedBlock &block);

// --- Macro inlining ---------------------------------------------------

static std::vector<AsmInst>
inlineMacroCall(const std::string &macroName,
                const std::vector<ast::FuncArg> &args) {
  auto it = macros.find(macroName);
  if (it == macros.end()) return {};

  const ast::Function &macro = *it->second;
  if (macro.args.size() != args.size()) {
    state.throwError("Macro '" + macroName + "' expects " +
                     std::to_string(macro.args.size()) +
                     " arguments, got " + std::to_string(args.size()) +
                     "!");
  }

  std::vector<AsmInst> res;
  state.pushScope("", "");
  for (size_t i = 0; i < args.size(); ++i) {
    state.declareVarAlias(macro.args[i].name, args[i].value);
  }

  auto body = scopedBlockToAsm(*macro.body);
  res.insert(res.end(), body.begin(), body.end());
  state.popScope();
  return res;
}

static const std::string LABEL_CMD_LOOP = "RSPQ_Loop";

// --- Type inference for declarations ----------------------------------

static std::string inferCalcResultType(const ast::Calc &calc,
                                        const std::string &declType) {
  return std::visit(
      [&](const auto &c) -> std::string {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, ast::CalcNum>) {
          return declType;
        } else if constexpr (std::is_same_v<T, ast::CalcVar>) {
          return declType;
        } else if constexpr (std::is_same_v<T, ast::CalcLR>) {
          const VarDef *l = state.getVar(c.left.value);
          if (!c.rightVarName.empty()) {
            const VarDef *r = state.getVar(c.rightVarName);
            if (l && r && (isVecType(l->type) || isVecType(r->type)))
              return isVecType(l->type) ? l->type : r->type;
          }
          if (l && isVecType(l->type)) return l->type;
          return declType;
        } else if constexpr (std::is_same_v<T, ast::CalcMulti>) {
          const VarDef *l = state.getVar(c.left.value);
          if (l && isVecType(l->type)) return l->type;
          for (const auto &p : c.parts) {
            if (!p.right.value.empty()) {
              const VarDef *r = state.getVar(p.right.value);
              if (r && isVecType(r->type)) return r->type;
            }
          }
          return declType;
        } else {
          return declType;
        }
      },
      calc);
}

// --- Forward declaration ----------------------------------------------

static std::vector<AsmInst>
calcToAsm(const ast::Calc &calc, const VarDef &varRes);

// --- Group decomposition for CalcMulti ---------------------------------

static std::vector<AsmInst>
decomposeCalcMulti(const ast::CalcMulti &cm, const VarDef &varRes) {
  // Fast path: single part with no groups -> treat as CalcLR
  if (cm.parts.size() == 1 && cm.groupStart == 0 &&
      cm.parts[0].groupStart == 0 && cm.parts[0].groupEnd == 0) {
    ast::CalcLR lrCalc;
    lrCalc.left = cm.left;
    lrCalc.swizzleLeft = cm.swizzleLeft;
    lrCalc.op = cm.parts[0].op;
    if (cm.parts[0].rightVal.has_value()) {
      lrCalc.rightNum = ast::ExprNum{cm.parts[0].rightVal.value()};
    } else {
      lrCalc.rightVarName = cm.parts[0].right.value;
    }
    lrCalc.swizzleRight = cm.parts[0].swizzleRight;
    return calcToAsm(ast::Calc(lrCalc), varRes);
  }

  // Fast path: all constants -> fold
  if (cm.leftVal.has_value()) {
    double acc = cm.leftVal.value();
    bool allConst = true;
    for (const auto &p : cm.parts) {
      if (!p.rightVal.has_value()) { allConst = false; break; }
      double r = p.rightVal.value();
      if (p.op == "+") acc += r;
      else if (p.op == "-") acc -= r;
      else if (p.op == "*") acc *= r;
      else if (p.op == "/" && r != 0) acc /= r;
      else if (p.op == "<<") acc = static_cast<int64_t>(acc) << static_cast<int>(r);
      else if (p.op == ">>") acc = static_cast<int64_t>(acc) >> static_cast<int>(r);
      else { allConst = false; break; }
    }
    if (allConst) {
      ast::CalcNum cn;
      cn.right = ast::ExprNum{acc};
      return calcToAsm(ast::Calc(cn), varRes);
    }
  }


  // Sequential handling for multi-part expressions without groups.
  bool hasGroups = (cm.groupStart > 0);
  for (const auto &p : cm.parts) {
    if (p.groupStart > 0 || p.groupEnd > 0) hasGroups = true;
  }
  if (hasGroups) {
    state.throwError(
        "Grouped expressions not yet supported — use intermediate variables");
  }

  std::vector<AsmInst> res;
  VarDef vLeft;
  if (cm.leftVal.has_value()) {
    vLeft.value = cm.leftVal.value();
    vLeft.type = varRes.type;
  } else {
    vLeft = state.getRequiredVarCopy(cm.left.value, "Left");
    vLeft.swizzle = cm.swizzleLeft;
  }

  bool accIsConst = cm.leftVal.has_value();
  int accConstVal = accIsConst ? static_cast<int>(cm.leftVal.value()) : 0;
  bool isFirst = true;
  for (const auto &part : cm.parts) {
    VarDef vRight;
    if (part.rightVal.has_value()) {
      vRight.value = part.rightVal.value();
      vRight.type = varRes.type;
    } else {
      vRight = state.getRequiredVarCopy(part.right.value, "right");
      vRight.swizzle = part.swizzleRight;
    }
    bool rightIsConst = part.rightVal.has_value();

    if (isFirst) {
      isFirst = false;
      if (accIsConst && rightIsConst) {
        if (part.op == "+") accConstVal += static_cast<int>(vRight.value);
        else if (part.op == "-") accConstVal -= static_cast<int>(vRight.value);
        else if (part.op == "*") accConstVal *= static_cast<int>(vRight.value);
        else if (part.op == "/") accConstVal /= static_cast<int>(vRight.value);
        continue;
      }
      if (accIsConst) {
        VarDef cl; cl.value = static_cast<double>(accConstVal);
        cl.type = varRes.type;
        auto mv = ops::opMove(varRes, cl);
        res.insert(res.end(), mv.begin(), mv.end());
        accIsConst = false;
      } else if (part.op == "+" && !isVecType(varRes.type)) {
        auto a = ops::opAdd(varRes, vLeft, vRight);
        res.insert(res.end(), a.begin(), a.end());
        continue;
      } else if (part.op == "+" && isVecType(varRes.type)) {
        auto a = ops::opAddVec(varRes, vLeft, vRight);
        res.insert(res.end(), a.begin(), a.end());
        continue;
      } else {
        auto mv = isVecType(varRes.type) ? ops::opMoveVec(varRes, vLeft)
                                          : ops::opMove(varRes, vLeft);
        res.insert(res.end(), mv.begin(), mv.end());
      }
    }

    if (!accIsConst) {
      bool isVec = isVecType(varRes.type);
      std::string op = part.op;
      if (isVec) {
        if (op == "+") { auto a = ops::opAddVec(varRes, varRes, vRight); res.insert(res.end(), a.begin(), a.end()); }
        else if (op == "-") { auto s = ops::opSubVec(varRes, varRes, vRight); res.insert(res.end(), s.begin(), s.end()); }
        else if (op == "*") { auto m = ops::opMulVec(varRes, varRes, vRight, true); res.insert(res.end(), m.begin(), m.end()); }
        else if (op == "&") { auto a = ops::opAndVec(varRes, varRes, vRight); res.insert(res.end(), a.begin(), a.end()); }
        else if (op == "|") { auto o = ops::opOrVec(varRes, varRes, vRight); res.insert(res.end(), o.begin(), o.end()); }
        else if (op == "^") { auto x = ops::opXORVec(varRes, varRes, vRight); res.insert(res.end(), x.begin(), x.end()); }
      } else {
        if (op == "+") { auto a = ops::opAdd(varRes, varRes, vRight); res.insert(res.end(), a.begin(), a.end()); }
        else if (op == "-") { auto s = ops::opSub(varRes, varRes, vRight); res.insert(res.end(), s.begin(), s.end()); }
        else if (op == "*") { auto m = ops::opMul(varRes, varRes, vRight); res.insert(res.end(), m.begin(), m.end()); }
        else if (op == "/") { auto d = ops::opDiv(varRes, varRes, vRight); res.insert(res.end(), d.begin(), d.end()); }
        else if (op == "&") { auto a = ops::opAnd(varRes, varRes, vRight); res.insert(res.end(), a.begin(), a.end()); }
        else if (op == "|") { auto o = ops::opOr(varRes, varRes, vRight); res.insert(res.end(), o.begin(), o.end()); }
        else if (op == "^") { auto x = ops::opXOR(varRes, varRes, vRight); res.insert(res.end(), x.begin(), x.end()); }
        else if (op == "<<") { auto s = ops::opShiftLeft(varRes, varRes, vRight); res.insert(res.end(), s.begin(), s.end()); }
        else if (op == ">>") { auto s = ops::opShiftRight(varRes, varRes, vRight, false); res.insert(res.end(), s.begin(), s.end()); }
        else if (op == ">>>") { auto s = ops::opShiftRight(varRes, varRes, vRight, true); res.insert(res.end(), s.begin(), s.end()); }
      }
    }
  }

  if (accIsConst && cm.parts.empty()) {
    VarDef cv; cv.value = static_cast<double>(accConstVal);
    cv.type = varRes.type;
    return isVecType(varRes.type) ? ops::opMoveVec(varRes, cv) : ops::opMove(varRes, cv);
  }
  return res;
}

// --- Calculation to ASM -----------------------------------------------

static std::vector<AsmInst>
calcToAsm(const ast::Calc &calc, const VarDef &varRes) {
  return std::visit(
      [&](const auto &c) -> std::vector<AsmInst> {
        using T = std::decay_t<decltype(c)>;

        if constexpr (std::is_same_v<T, ast::CalcNum>) {
          VarDef vRight;
          vRight.value = c.right.value;
          if (isVecType(varRes.type)) {
            return ops::opMoveVec(varRes, vRight);
          }
          return ops::opMove(varRes, vRight);
        }

        else if constexpr (std::is_same_v<T, ast::CalcVar>) {
          VarDef vRight =
              state.getRequiredVarCopy(c.right.value, "right");
          vRight.swizzle = c.swizzleRight;
          if (c.op == "~") {
            if (isVecType(varRes.type))
              return ops::opBitFlipVec(varRes, vRight);
            return ops::opBitFlip(varRes, vRight);
          }
          if (isVecType(varRes.type)) {
            return ops::opMoveVec(varRes, vRight);
          }
          return ops::opMove(varRes, vRight);
        }

        else if constexpr (std::is_same_v<T, ast::CalcLR>) {
          VarDef vLeft =
              state.getRequiredVarCopy(c.left.value, "Left");
          vLeft.swizzle = c.swizzleLeft;

          VarDef vRight;
          if (!c.rightVarName.empty()) {
            vRight =
                state.getRequiredVarCopy(c.rightVarName, "right");
          } else {
            vRight.value = c.rightNum.value;
            vRight.type = varRes.type;
          }
          vRight.swizzle = c.swizzleRight;

          bool isVec = isVecType(varRes.type);
          std::string op = c.op;

          if (!isVec) {
            if (!c.swizzleLeft.empty() && !vLeft.reg.empty() &&
                !isVecType(vLeft.type))
              state.throwError(
                  "Swizzling not allowed for scalar operations!");
            if (!c.swizzleRight.empty() && !vRight.reg.empty() &&
                !isVecType(vRight.type))
              state.throwError(
                  "Swizzling not allowed for scalar operations!");
          }

          if (isVec) {
            if (op == "+") return ops::opAddVec(varRes, vLeft, vRight);
            if (op == "-") return ops::opSubVec(varRes, vLeft, vRight);
            if (op == "*" || op == "+*")
              return ops::opMulVec(varRes, vLeft, vRight, op == "*");
            if (op == "&") return ops::opAndVec(varRes, vLeft, vRight);
            if (op == "|") return ops::opOrVec(varRes, vLeft, vRight);
            if (op == "^") return ops::opXORVec(varRes, vLeft, vRight);
            if (op == "<<")
              return ops::opShiftLeftVec(varRes, vLeft, vRight);
            if (op == ">>")
              return ops::opShiftRightVec(varRes, vLeft, vRight, false);
            if (op == ">>>")
              return ops::opShiftRightVec(varRes, vLeft, vRight, true);
          } else {
            if (op == "+") return ops::opAdd(varRes, vLeft, vRight);
            if (op == "-") return ops::opSub(varRes, vLeft, vRight);
            if (op == "*") return ops::opMul(varRes, vLeft, vRight);
            if (op == "/") return ops::opDiv(varRes, vLeft, vRight);
            if (op == "&") return ops::opAnd(varRes, vLeft, vRight);
            if (op == "|") return ops::opOr(varRes, vLeft, vRight);
            if (op == "^") return ops::opXOR(varRes, vLeft, vRight);
            if (op == "~|") return ops::opNOR(varRes, vLeft, vRight);
            if (op == "<<")
              return ops::opShiftLeft(varRes, vLeft, vRight);
            if (op == ">>")
              return ops::opShiftRight(varRes, vLeft, vRight, false);
            if (op == ">>>")
              return ops::opShiftRight(varRes, vLeft, vRight, true);
          }
          state.throwError("Unknown operator: " + op);
          return {};
        }

        else if constexpr (std::is_same_v<T, ast::CalcMulti>) {
          return decomposeCalcMulti(c, varRes);
        }

        else if constexpr (std::is_same_v<T, ast::CalcFunc>) {
          if (macros.count(c.funcName)) {
            std::vector<ast::FuncArg> callArgs;
            callArgs.push_back(
                {.type = "var", .value = varRes.name, .swizzle = ""});
            for (auto &a : c.args) callArgs.push_back(a);
            return inlineMacroCall(c.funcName, callArgs);
          }
          auto *bf = builtins::lookup(c.funcName);
          if (!bf)
            state.throwError("Unknown builtin: " + c.funcName);
          VarDef resCopy = varRes;
          return (*bf)(&resCopy, c.args, c.swizzleRight);
        }

        else if constexpr (std::is_same_v<T, ast::CalcCompare>) {
          bool isVec = isVecType(varRes.type);
          if (isVec) {
            VarDef vLeft =
                state.getRequiredVarCopy(c.left, "left");
            VarDef vRight;
            if (c.rightVal.has_value()) {
              auto pIt =
                  POW2_SWIZZLE_VAR.find(c.rightVal.value());
              if (pIt == POW2_SWIZZLE_VAR.end())
                state.throwError("Constant must be a power of two!");
              vRight.reg = pIt->second.reg;
              vRight.swizzle = pIt->second.swizzle;
              vRight.type = "vec16";
            } else {
              vRight = state.getRequiredVarCopy(c.right, "right");
              vRight.swizzle = c.swizzleRight;
            }
            const ast::TernaryPart *tp =
                c.ternary.has_value() ? &c.ternary.value() : nullptr;
            return ops::opCompareVec(varRes, vLeft, vRight, c.op, tp);
          } else {
            VarDef vLeft =
                state.getRequiredVarCopy(c.left, "left");
            VarDef vRight;
            if (c.rightVal.has_value()) {
              vRight.value = c.rightVal.value();
              vRight.type = varRes.type;
            } else {
              vRight = state.getRequiredVarCopy(c.right, "right");
            }
            return ops::opCompare(varRes, vLeft, vRight, c.op, false);
          }
        }

        state.throwError("Unknown calculation type");
        return {};
      },
      calc);
}

// --- Control flow -----------------------------------------------------

static std::vector<AsmInst> ifToAsm(const ast::StmtIf &st) {
  const VarDef *varLeft =
      state.getRequiredVar(st.compare.left.value, "left");
  if (reg::isVecReg(varLeft->reg))
    state.throwError("IF-Statements must use scalar-registers!");

  std::string labelElse = state.generateLabel();
  std::string labelEnd =
      st.blockElse ? state.generateLabel() : labelElse;

  std::vector<AsmInst> res;
  auto branch = ops::opBranch(st.compare, labelElse);
  res.insert(res.end(), branch.begin(), branch.end());

  state.pushScope("", "");
  auto ifBlock = scopedBlockToAsm(*st.blockIf);
  res.insert(res.end(), ifBlock.begin(), ifBlock.end());
  if (st.blockElse) {
    res.push_back(
        asmBranch("beq", {"$zero", "$zero", labelEnd}, labelEnd));
    res.push_back(asmNOP());
  }
  state.popScope();

  if (st.blockElse) {
    state.pushScope("", labelElse);
    res.push_back(asmLabel(labelElse));
    auto elseBlock = scopedBlockToAsm(*st.blockElse);
    res.insert(res.end(), elseBlock.begin(), elseBlock.end());
    state.popScope();
  }
  res.push_back(asmLabel(labelEnd));
  return res;
}

static std::vector<AsmInst> whileToAsm(const ast::StmtWhile &st) {
  const VarDef *varLeft =
      state.getRequiredVar(st.compare.left.value, "left");
  if (reg::isVecReg(varLeft->reg))
    state.throwError("While-Statements must use scalar-registers!");

  std::string labelStart = state.generateLabel();
  std::string labelEnd = state.generateLabel();

  std::vector<AsmInst> res;
  res.push_back(asmLabel(labelStart));

  auto branch = ops::opBranch(st.compare, labelEnd);
  res.insert(res.end(), branch.begin(), branch.end());

  state.pushScope(labelStart, labelEnd);
  auto body = scopedBlockToAsm(*st.block);
  res.insert(res.end(), body.begin(), body.end());
  state.popScope();

  res.push_back(asmOp("j", {labelStart}));
  res.push_back(asmNOP());
  res.push_back(asmLabel(labelEnd));
  return res;
}

static std::vector<AsmInst> loopToAsm(const ast::StmtLoop &st) {
  std::string labelStart = state.generateLabel();
  std::string labelEnd = state.generateLabel();

  std::vector<AsmInst> res;
  res.push_back(asmLabel(labelStart));
  state.pushScope(labelStart, labelEnd);
  auto body = scopedBlockToAsm(*st.block);
  res.insert(res.end(), body.begin(), body.end());
  state.popScope();
  res.push_back(asmOp("j", {labelStart}));
  res.push_back(asmNOP());
  res.push_back(asmLabel(labelEnd));
  return res;
}

// --- Statement dispatch ------------------------------------------------

static std::vector<AsmInst>
scopedBlockToAsm(const ast::ScopedBlock &block) {
  std::vector<AsmInst> res;

  for (const auto &stmt : block.statements) {
    std::visit(
        [&](const auto &s) {
          using T = std::decay_t<decltype(s)>;

          if constexpr (std::is_same_v<T, ast::StmtVarDecl>) {
            std::string reg = s.reg.empty()
                                  ? state.allocRegister(s.varType)
                                  : s.reg;
            state.declareVar(s.varName, s.varType, reg,
                             s.isConst);
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtVarDeclMulti>) {
            for (size_t i = 0; i < s.varNames.size(); ++i) {
              int step = isTwoRegType(s.varType) ? 2 : 1;
              int offset = static_cast<int>(i) * step;
              std::string reg = s.reg.empty()
                                    ? state.allocRegister(s.varType)
                                    : reg::nextReg(s.reg, offset)
                                          ? *reg::nextReg(s.reg, offset)
                                          : s.reg;
              state.declareVar(s.varNames[i], s.varType, reg,
                               s.isConst);
            }
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtVarDeclAssign>) {
            std::string baseName =
                s.varName.substr(0, s.varName.find(':'));
            std::string effectiveType = s.varType;
            if (s.calc) {
              effectiveType = inferCalcResultType(*s.calc, s.varType);
            }
            state.declareVar(baseName, effectiveType,
                             s.reg.empty()
                                 ? state.allocRegister(effectiveType)
                                 : s.reg,
                             s.isConst);
            if (s.calc) {
              VarDef vr = state.getRequiredVarCopy(
                  s.varName, "result");
              auto calcAsm = calcToAsm(*s.calc, vr);
              res.insert(res.end(), calcAsm.begin(),
                         calcAsm.end());
              state.markVarModified(baseName);
            }
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtVarDeclAlias>) {
            state.declareVarAlias(s.aliasName, s.varName);
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtVarUndef>) {
            state.undefVar(s.varName);
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtVarAssignCalc>) {
            bool handledAsFuncCall = false;
            if (auto *cf = std::get_if<ast::CalcFunc>(s.calc.get())) {
              if (!builtins::lookup(cf->funcName)) {
                std::vector<ast::FuncArg> callArgs;
                callArgs.push_back(
                    {.type = "var", .value = s.varName, .swizzle = s.swizzle});
                for (auto &a : cf->args) callArgs.push_back(a);
                if (macros.count(cf->funcName)) {
                  auto inlineRes = inlineMacroCall(cf->funcName, callArgs);
                  res.insert(res.end(), inlineRes.begin(), inlineRes.end());
                } else {
                  auto callRes = ops::callUserFunction(cf->funcName, callArgs);
                  res.insert(res.end(), callRes.begin(), callRes.end());
                }
                handledAsFuncCall = true;
              }
            }
            if (!handledAsFuncCall) {
              VarDef vr =
                  state.getRequiredVarCopy(s.varName, "result");
              vr.swizzle = s.swizzle;

              if (vr.isConst && vr.modifyCount > 0) {
                state.throwError("Cannot assign to constant variable!");
              }
              state.markVarModified(s.varName);

              std::string op = s.assignType;
              if (op.size() >= 1 && op != "=") {
                std::string baseOp = op.substr(0, op.size() - 1);
                if (auto *cn = std::get_if<ast::CalcNum>(s.calc.get())) {
                  ast::CalcLR lrCalc;
                  lrCalc.left = ast::ExprVarName{s.varName};
                  lrCalc.op = baseOp;
                  lrCalc.rightNum = cn->right;
                  auto calcAsm = calcToAsm(ast::Calc(lrCalc), vr);
                  res.insert(res.end(), calcAsm.begin(), calcAsm.end());
                } else if (auto *cv = std::get_if<ast::CalcVar>(s.calc.get())) {
                  ast::CalcLR lrCalc;
                  lrCalc.left = ast::ExprVarName{s.varName};
                  lrCalc.swizzleLeft = s.swizzle;
                  lrCalc.op = baseOp;
                  lrCalc.rightVarName = cv->right.value;
                  lrCalc.swizzleRight = cv->swizzleRight;
                  auto calcAsm = calcToAsm(ast::Calc(lrCalc), vr);
                  res.insert(res.end(), calcAsm.begin(), calcAsm.end());
                } else {
                  auto calcAsm = calcToAsm(*s.calc, vr);
                  res.insert(res.end(), calcAsm.begin(), calcAsm.end());
                }
              } else {
                auto calcAsm = calcToAsm(*s.calc, vr);
                res.insert(res.end(), calcAsm.begin(), calcAsm.end());
              }
            }
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtFuncCall>) {
            if (macros.count(s.func)) {
              auto inlineRes = inlineMacroCall(s.func, s.args);
              res.insert(res.end(), inlineRes.begin(), inlineRes.end());
            } else {
              auto *bf = builtins::lookup(s.func);
              if (bf) {
                auto callRes = (*bf)(nullptr, s.args, "");
                res.insert(res.end(), callRes.begin(), callRes.end());
              } else {
                auto callRes = ops::callUserFunction(s.func, s.args);
                res.insert(res.end(), callRes.begin(), callRes.end());
              }
            }
          }

          else if constexpr (std::is_same_v<T, ast::StmtLabelDecl>) {
            res.push_back(asmLabel(s.name));
          }

          else if constexpr (std::is_same_v<T, ast::StmtGoto>) {
            res.push_back(asmOp("j", {s.label}));
            res.push_back(asmNOP());
          }

          else if constexpr (std::is_same_v<T, ast::StmtIf>) {
            auto ifRes = ifToAsm(s);
            res.insert(res.end(), ifRes.begin(), ifRes.end());
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtWhile>) {
            auto wRes = whileToAsm(s);
            res.insert(res.end(), wRes.begin(), wRes.end());
          }

          else if constexpr (std::is_same_v<T, ast::StmtLoop>) {
            auto lRes = loopToAsm(s);
            res.insert(res.end(), lRes.begin(), lRes.end());
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtBreak>) {
            const Scope &scope = state.getScope();
            if (!scope.labelEnd.empty()) {
              res.push_back(asmOp("j", {scope.labelEnd}));
              res.push_back(asmNOP());
            }
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtContinue>) {
            const Scope &scope = state.getScope();
            if (!scope.labelStart.empty()) {
              res.push_back(asmOp("j", {scope.labelStart}));
              res.push_back(asmNOP());
            }
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtExit>) {
            res.push_back(asmOp("j", {LABEL_CMD_LOOP}));
            res.push_back(asmNOP());
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtAnnotation>) {
            state.addAnnotation(s.name, s.value);
          }

          else if constexpr (std::is_same_v<T,
                                             ast::StmtScopedBlock>) {
            state.pushScope("", "");
            auto body = scopedBlockToAsm(*s.body);
            res.insert(res.end(), body.begin(), body.end());
            state.popScope();
          }
        },
        stmt);
  }

  return res;
}

// --- ast2asm main -----------------------------------------------------

std::vector<AsmFunc> ast2asm(const ast::Program &ast) {
  std::vector<AsmFunc> result;
  state.reset();

  // Register macros
  macros.clear();
  for (const auto &fn : ast.functions) {
    if (fn.type == "macro") {
      macros[fn.name] = &fn;
    }
  }

  // Pre-declare memory variables from state/data/bss sections
  for (const auto &sec : ast.states) {
    for (const auto &sv : sec.vars) {
      if (sv.isExtern) continue;
      int64_t arraySize = 1;
      for (auto dim : sv.arraySize)
        arraySize *= dim;
      if (arraySize < 1) arraySize = 1;
      state.declareMemVar(sv.varName, sv.varType,
                          static_cast<int>(arraySize));
    }
  }

  // Pre-declare all functions so they can reference each other
  for (const auto &fn : ast.functions) {
    if (fn.type == "function" || fn.type == "command") {
      bool isRelative = false;
      for (const auto &ann : fn.annotations) {
        if (ann.name == "Relative") isRelative = true;
      }
      state.declareFunction(fn.name, fn.args, isRelative);
    }
  }

  for (const auto &fn : ast.functions) {
    if (fn.type == "macro") continue; // already registered

    state.enterFunction(fn.name, fn.type, fn.resultType.value_or(0));

    bool isCommand = (fn.type == "command");
    // Built-in registers (ZERO, VZERO, RA, etc.) are already
    // declared by enterFunction().

    // Declare function arguments
    int argSize = 0;
    for (const auto &arg : fn.args) {
      std::string reg;
      if (!arg.reg.empty()) {
        reg = arg.reg;
      } else {
        reg = state.allocRegister(arg.type);
      }
      state.declareVar(arg.name, arg.type, reg);
      argSize++;
    }

    std::vector<AsmInst> funcAsm;
    auto body = scopedBlockToAsm(*fn.body);
    funcAsm.insert(funcAsm.end(), body.begin(), body.end());

    if (isCommand) {
      funcAsm.push_back(asmOp("j", {LABEL_CMD_LOOP}));
      funcAsm.push_back(asmNOP());
    } else {
      funcAsm.push_back(asmOp("jr", {reg::Reg::RA}));
      funcAsm.push_back(asmNOP());
    }

    AsmFunc af;
    af.name = fn.name;
    af.type = fn.type;
    af.asm_ = std::move(funcAsm);
    af.argSize = argSize;
    af.resultType = fn.resultType.value_or(0);
    for (const auto &ann : fn.annotations) {
      af.annotations.push_back({ann.name, ann.value});
    }

    normalizeASM(af);

    result.push_back(std::move(af));
    state.leaveFunction();
  }

  return result;
}

} // namespace rspl

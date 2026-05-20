#include "ast2asm.h"

#include "asm.h"
#include "asm_normalize.h"
#include "ast.h"
#include "astCalcNormalizer.h"
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
#include <cmath>
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
          // If the declared type is already a vector type, trust it.
          // This matters for mixed-type expressions like vec16 * vec32
          // whose result should be vec32 if the variable is declared as
          // vec32. Without this the left operand's type (vec16) wins and
          // later type-sensitive operations (e.g. clip()) receive wrong
          // types.
          if (isVecType(declType)) return declType;
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
// Port of JS astCalcNormalizer + astCalcPartsToASM.
// Flattens group markers, applies precedence, constant-folds (via
// partsEval), and decomposes complex expressions into temp variables.

// Flatten calcMulti parts + group markers into a linear token vector.
static std::vector<FlatElem> flattenCalcMulti(const ast::CalcMulti &cm) {
  std::vector<FlatElem> out;
  // Use a special OP-like sentinel for brackets so partsToTree can find them.
  FlatElem lparen, rparen;
  lparen.kind = FlatElem::OP;
  lparen.opStr = "(";
  rparen.kind = FlatElem::OP;
  rparen.opStr = ")";

  auto addBrackets = [&](int count, const FlatElem &b) {
    for (int i = 0; i < count; ++i) out.push_back(b);
  };
  addBrackets(cm.groupStart, lparen);
  if (cm.leftVal.has_value()) {
    out.push_back({FlatElem::VAL, {}, cm.leftVal.value(), {},
                   cm.swizzleLeft, true});
  } else {
    out.push_back({FlatElem::VAL, {}, 0, cm.left.value,
                   cm.swizzleLeft, false});
  }
  for (const auto &p : cm.parts) {
    out.push_back({FlatElem::OP, p.op});
    addBrackets(p.groupStart, lparen);
    if (p.rightVal.has_value()) {
      out.push_back({FlatElem::VAL, {}, p.rightVal.value(), {},
                     p.swizzleRight, true});
    } else {
      out.push_back({FlatElem::VAL, {}, 0, p.right.value,
                     p.swizzleRight, false});
    }
    addBrackets(p.groupEnd, rparen);
  }
  return out;
}

// Convert bracket markers "(" / ")" into nested FlatElem vectors.
static void partsToTree(std::vector<FlatElem> &parts) {
  for (size_t i = 0; i < parts.size();) {
    if (parts[i].kind == FlatElem::OP && parts[i].opStr == "(") {
      int depth = 1;
      size_t start = i;
      size_t j = i + 1;
      for (; j < parts.size() && depth > 0; ++j) {
        if (parts[j].kind == FlatElem::OP && parts[j].opStr == "(") depth++;
        else if (parts[j].kind == FlatElem::OP && parts[j].opStr == ")") depth--;
      }
      // Extract sub-expression between brackets
      std::vector<FlatElem> sub(parts.begin() + start + 1,
                                parts.begin() + j - 1);
      partsToTree(sub); // recurse into sub-expression
      // Replace the bracket group with a single nested FlatElem
      parts.erase(parts.begin() + start, parts.begin() + j);
      FlatElem nested;
      nested.kind = FlatElem::VAL;
      nested.varName = NESTED_SENTINEL;
      nested.nested = std::move(sub);
      nested.isNested = true;
      parts.insert(parts.begin() + start, std::move(nested));
      i = start + 1; // continue after the inserted element
    } else {
      ++i;
    }
  }
}

// Forward declaration for mutual recursion.
static void decomposeParts(std::vector<FlatElem> &parts,
                           const VarDef &varRes,
                           std::vector<AsmInst> &out,
                           int &tmpCounter);

// Resolve a FlatElem value into a VarDef.  For nested sub-expressions,
// recursively decompose into temp variables and return the temp var.
static VarDef resolveFlatVal(FlatElem &elem,
                             const VarDef &varRes,
                             std::vector<AsmInst> &out,
                             int &tmpCounter) {
  if (elem.isNested) {
    // Recursively decompose the nested sub-expression into a temp variable
    std::string tmpName = "__tmp_" + std::to_string(tmpCounter++);
    state.declareVar(tmpName, varRes.type,
                     state.allocRegister(varRes.type));
    VarDef tmpVar = state.getRequiredVarCopy(tmpName, "tmp");
    decomposeParts(elem.nested, tmpVar, out, tmpCounter);
    return tmpVar;
  }
  if (elem.isNum) {
    VarDef v;
    v.value = elem.numVal;
    v.type = varRes.type;
    return v;
  }
  VarDef v = state.getRequiredVarCopy(elem.varName, "val");
  v.swizzle = elem.swizzle;
  return v;
}

// Decompose a parts vector into ASM instructions, accumulating into
// `varRes`.  Nested sub-expressions are emitted into temp variables.
static void decomposeParts(std::vector<FlatElem> &parts,
                           const VarDef &varRes,
                           std::vector<AsmInst> &out,
                           int &tmpCounter) {
  if (parts.empty()) return;

  // Resolve first value
  size_t pos = 0;
  VarDef accVar;
  bool accIsConst = false;
  double accConst = 0;

  if (pos < parts.size() && parts[pos].kind == FlatElem::VAL) {
    if (parts[pos].isNested) {
      accVar = resolveFlatVal(parts[pos], varRes, out, tmpCounter);
    } else if (parts[pos].isNum) {
      accIsConst = true;
      accConst = parts[pos].numVal;
    } else {
      accVar = state.getRequiredVarCopy(parts[pos].varName, "left");
      accVar.swizzle = parts[pos].swizzle;
    }
    ++pos;
  }

  if (pos >= parts.size()) {
    VarDef finalLeft;
    if (accIsConst) {
      finalLeft.value = accConst;
      finalLeft.type = varRes.type;
    } else {
      finalLeft = accVar;
    }
    auto mv = isVecType(varRes.type) ? ops::opMoveVec(varRes, finalLeft)
                                      : ops::opMove(varRes, finalLeft);
    out.insert(out.end(), mv.begin(), mv.end());
    return;
  }

  bool isFirst = true;
  VarDef firstLeft;
  if (!accIsConst) firstLeft = accVar;

  while (pos + 1 <= parts.size() &&
         (pos < parts.size() && parts[pos].kind == FlatElem::OP)) {
    std::string op = parts[pos].opStr;
    // Skip bracket sentinels (shouldn't appear after partsToTree)
    if (op == "(" || op == ")") { ++pos; continue; }
    ++pos;
    VarDef right;
    if (pos < parts.size() && parts[pos].kind == FlatElem::VAL) {
      right = resolveFlatVal(parts[pos], varRes, out, tmpCounter);
      ++pos;
    }

    if (isFirst) {
      isFirst = false;
      if (accIsConst) {
        VarDef cl;
        cl.value = accConst;
        cl.type = varRes.type;
        auto mv = isVecType(varRes.type) ? ops::opMoveVec(varRes, cl)
                                          : ops::opMove(varRes, cl);
        out.insert(out.end(), mv.begin(), mv.end());
        accIsConst = false;
        // Apply op to varRes
        if (!isVecType(varRes.type)) {
          if (op == "+") { auto a = ops::opAdd(varRes, varRes, right); out.insert(out.end(), a.begin(), a.end()); }
          else if (op == "-") { auto s = ops::opSub(varRes, varRes, right); out.insert(out.end(), s.begin(), s.end()); }
          else if (op == "*") { auto m = ops::opMul(varRes, varRes, right); out.insert(out.end(), m.begin(), m.end()); }
        }
      } else {
        // Try to fuse move + first op by calling the appropriate op directly
        if (!isVecType(varRes.type)) {
          if (op == "+") {
            auto a = ops::opAdd(varRes, firstLeft, right);
            out.insert(out.end(), a.begin(), a.end());
          } else if (op == "-") {
            auto s = ops::opSub(varRes, firstLeft, right);
            out.insert(out.end(), s.begin(), s.end());
          } else if (op == "*") {
            auto m = ops::opMul(varRes, firstLeft, right);
            out.insert(out.end(), m.begin(), m.end());
          } else if (op == "/") {
            auto d = ops::opDiv(varRes, firstLeft, right);
            out.insert(out.end(), d.begin(), d.end());
          } else if (op == "&") {
            auto a = ops::opAnd(varRes, firstLeft, right);
            out.insert(out.end(), a.begin(), a.end());
          } else if (op == "|") {
            auto o = ops::opOr(varRes, firstLeft, right);
            out.insert(out.end(), o.begin(), o.end());
          } else if (op == "^") {
            auto x = ops::opXOR(varRes, firstLeft, right);
            out.insert(out.end(), x.begin(), x.end());
          } else if (op == "<<") {
            auto s = ops::opShiftLeft(varRes, firstLeft, right);
            out.insert(out.end(), s.begin(), s.end());
          } else if (op == ">>") {
            auto s = ops::opShiftRight(varRes, firstLeft, right, false);
            out.insert(out.end(), s.begin(), s.end());
          } else if (op == ">>>") {
            auto s = ops::opShiftRight(varRes, firstLeft, right, true);
            out.insert(out.end(), s.begin(), s.end());
          } else {
            // Unknown op: move then apply
            auto mv = ops::opMove(varRes, firstLeft);
            out.insert(out.end(), mv.begin(), mv.end());
          }
        } else {
          // Vec ops
          if (op == "+") {
            auto a = ops::opAddVec(varRes, firstLeft, right);
            out.insert(out.end(), a.begin(), a.end());
          } else if (op == "-") {
            auto s = ops::opSubVec(varRes, firstLeft, right);
            out.insert(out.end(), s.begin(), s.end());
          } else if (op == "*") {
            auto m = ops::opMulVec(varRes, firstLeft, right, true);
            out.insert(out.end(), m.begin(), m.end());
          } else if (op == "+*") {
            auto m = ops::opMulVec(varRes, firstLeft, right, false);
            out.insert(out.end(), m.begin(), m.end());
          } else {
            auto mv = ops::opMoveVec(varRes, firstLeft);
            out.insert(out.end(), mv.begin(), mv.end());
          }
        }
      }
    } else {
      if (!isVecType(varRes.type)) {
        if (op == "+") { auto a = ops::opAdd(varRes, varRes, right); out.insert(out.end(), a.begin(), a.end()); }
        else if (op == "-") { auto s = ops::opSub(varRes, varRes, right); out.insert(out.end(), s.begin(), s.end()); }
        else if (op == "*") { auto m = ops::opMul(varRes, varRes, right); out.insert(out.end(), m.begin(), m.end()); }
        else if (op == "/") { auto d = ops::opDiv(varRes, varRes, right); out.insert(out.end(), d.begin(), d.end()); }
        else if (op == "&") { auto a = ops::opAnd(varRes, varRes, right); out.insert(out.end(), a.begin(), a.end()); }
        else if (op == "|") { auto o = ops::opOr(varRes, varRes, right); out.insert(out.end(), o.begin(), o.end()); }
        else if (op == "^") { auto x = ops::opXOR(varRes, varRes, right); out.insert(out.end(), x.begin(), x.end()); }
        else if (op == "<<") { auto s = ops::opShiftLeft(varRes, varRes, right); out.insert(out.end(), s.begin(), s.end()); }
        else if (op == ">>") { auto s = ops::opShiftRight(varRes, varRes, right, false); out.insert(out.end(), s.begin(), s.end()); }
        else if (op == ">>>") { auto s = ops::opShiftRight(varRes, varRes, right, true); out.insert(out.end(), s.begin(), s.end()); }
      }
    }
    accIsConst = false;
    accVar = varRes;
  }
}

static std::vector<AsmInst>
decomposeCalcMulti(const ast::CalcMulti &cm, const VarDef &varRes) {
  // Fast path: single part, no groups, variable left
  if (cm.parts.size() == 1 && cm.groupStart == 0 &&
      cm.parts[0].groupStart == 0 && cm.parts[0].groupEnd == 0 &&
      !cm.leftVal.has_value()) {
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

  // Step 1: flatten group markers into bracket tokens
  auto parts = flattenCalcMulti(cm);

  // Step 2: convert brackets to nested structure
  partsToTree(parts);

  // Step 2.5: apply operator precedence within nested groups
  for (auto &e : parts) {
    if (e.isNested) applyPrecedence(e.nested);
  }

  // Step 3: evaluate constant sub-expressions (delegated to partsEval)
  auto evalResult = partsEval(parts);
  if (std::holds_alternative<FlatElem>(evalResult)) {
    // Entire expression folded to a single constant
    FlatElem &elem = std::get<FlatElem>(evalResult);
    VarDef v;
    v.value = elem.numVal;
    v.type = varRes.type;
    return isVecType(varRes.type) ? ops::opMoveVec(varRes, v)
                                   : ops::opMove(varRes, v);
  }
  // partsEval returned the (possibly modified) parts vector
  parts = std::get<std::vector<FlatElem>>(std::move(evalResult));

  // Step 4: decompose into temp variables
  std::vector<AsmInst> res;
  int tmpCounter = 0;
  decomposeParts(parts, varRes, res, tmpCounter);
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
          // Check if the variable is actually a label / state memory
          // (JS: astNormalize.js lines 161-166)
          const auto *memVar = state.getMemVarOrNull(c.right.value);
          if (memVar) {
            // Convert label reference to %lo(NAME) immediate
            VarDef vRight;
            vRight.value = 0;
            vRight.type = varRes.type;
            vRight.reg = "%lo(" + c.right.value + ")";
            if (isVecType(varRes.type))
              return ops::opMoveVec(varRes, vRight);
            return ops::opMove(varRes, vRight);
          }
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
            // Check for label / state-memory reference (JS: astNormalize.js:161-166)
            const auto *memVar =
                state.getMemVarOrNull(c.rightVarName);
            if (memVar) {
              vRight.value = 0;
              vRight.type = varRes.type;
              vRight.reg = "%lo(" + c.rightVarName + ")";
            } else {
              vRight = state.getRequiredVarCopy(c.rightVarName, "right");
            }
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

  // loop { body } while(cond) — emit conditional branch at the tail
  if (st.compare.has_value()) {
        if (st.compare->left.type == "num") {
      state.throwError(
          "Loop-Statements with numeric left-hand-side not implemented!");
    }
    const VarDef *varLeft =
        state.getRequiredVar(st.compare->left.value, "left");
    if (reg::isVecReg(varLeft->reg))
      state.throwError("Loop-Statements must use scalar-registers!");

    std::vector<AsmInst> res;
    res.push_back(asmLabel(labelStart));
    state.pushScope(labelStart, labelEnd);
    auto body = scopedBlockToAsm(*st.block);
    res.insert(res.end(), body.begin(), body.end());
    auto branchOps =
        ops::opBranch(*st.compare, labelStart, /*invert=*/true);
    res.insert(res.end(), branchOps.begin(), branchOps.end());
    state.popScope();
    res.push_back(asmLabel(labelEnd));
    return res;
  }

  // Infinite loop: j back to start
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

// Pre-scan scoped blocks for label declarations and register them as
// memory variables.  Ported from JS astNormalize.js lines 19-28.
static void predeclareLabels(const ast::ScopedBlock &block) {
  for (const auto &stmt : block.statements) {
    if (std::holds_alternative<ast::StmtLabelDecl>(stmt)) {
      auto &ld = std::get<ast::StmtLabelDecl>(stmt);
      state.declareMemVar(ld.name, "u16", 1);
    } else if (auto *sb = std::get_if<ast::StmtScopedBlock>(&stmt)) {
      predeclareLabels(*sb->body);
    } else if (auto *si = std::get_if<ast::StmtIf>(&stmt)) {
      if (si->blockIf) predeclareLabels(*si->blockIf);
      if (si->blockElse) predeclareLabels(*si->blockElse);
    } else if (auto *sw = std::get_if<ast::StmtWhile>(&stmt)) {
      if (sw->block) predeclareLabels(*sw->block);
    } else if (auto *sl = std::get_if<ast::StmtLoop>(&stmt)) {
      if (sl->block) predeclareLabels(*sl->block);
    }
  }
}

static std::vector<AsmInst>
scopedBlockToAsm(const ast::ScopedBlock &block) {
  state.line = block.line;
  // Pre-scan labels so forward references resolve (JS: astNormalize.js:19-28)
  predeclareLabels(block);

  std::vector<AsmInst> res;

  for (const auto &stmt : block.statements) {
    // Update state.line from the statement's line number (for debug info)
    std::visit([&](const auto &s) { state.line = s.line; }, stmt);

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
                } else if (auto *cm = std::get_if<ast::CalcMulti>(s.calc.get())) {
                  // Wrap compound assign: a += expr  ->  a = a + (expr)
                  // Structure: left=a, part0 opens group for expr.
                  ast::CalcMulti wrapped;
                  wrapped.left = ast::ExprVarName{s.varName};
                  wrapped.swizzleLeft = s.swizzle;
                  // First part: a + (expr) - open a bracket for expr
                  ast::CalcMultiPart firstPart;
                  firstPart.op = baseOp;
                  if (cm->leftVal.has_value()) {
                    firstPart.rightVal = cm->leftVal;
                  } else {
                    firstPart.right = cm->left;
                  }
                  firstPart.swizzleRight = cm->swizzleLeft;
                  firstPart.groupStart = 1 + cm->groupStart; // open sub-expr
                  firstPart.groupEnd = 0;
                  wrapped.parts.push_back(std::move(firstPart));
                  // Copy original parts, closing the bracket at the end
                  for (auto &p : cm->parts) {
                    wrapped.parts.push_back(p);
                  }
                  wrapped.parts.back().groupEnd += 1; // close sub-expr
                  auto calcAsm = calcToAsm(ast::Calc(wrapped), vr);
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

    // Clear per-statement annotations (matching JS ast2asm.js:394-395)
    // Annotation statements themselves are exempt so their annotations
    // apply to the next real statement.
    if (!std::holds_alternative<ast::StmtAnnotation>(stmt))
      state.clearAnnotations();
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
    if (!fn.body) continue; // forward declaration only — no body to generate

    // argSize in bytes, matching JS getArgSize() = max(args.length * 4, 4)
    int byteArgSize =
        std::max(static_cast<int>(fn.args.size()) * 4, 4);
    state.enterFunction(fn.name, fn.type,
                        fn.type == "command" ? byteArgSize
                                             : fn.resultType.value_or(0));

    bool isCommand = (fn.type == "command");
    // Built-in registers (ZERO, VZERO, RA, etc.) are already
    // declared by enterFunction().

    // Declare function arguments
    int argSize = 0;
    static const char *argRegs[] = {reg::Reg::A0, reg::Reg::A1,
                                    reg::Reg::A2, reg::Reg::A3};
    for (const auto &arg : fn.args) {
      std::string reg;
      if (!arg.reg.empty()) {
        reg = arg.reg;
      } else if (argSize < 4) {
        reg = argRegs[argSize];
      } else {
        reg = state.allocRegister(arg.type);
      }
      state.declareVar(arg.name, arg.type, reg);
      argSize++;
    }

    std::vector<AsmInst> funcAsm;
    auto body = scopedBlockToAsm(*fn.body);
    funcAsm.insert(funcAsm.end(), body.begin(), body.end());

    // Advance past the closing brace (matching JS ast2asm.js:443)
    ++state.line;

    // Check @NoReturn annotation (matching JS ast2asm.js:445)
    bool needsReturn = true;
    for (const auto &ann : fn.annotations) {
      if (ann.name == "NoReturn") needsReturn = false;
    }
    if (needsReturn) {
      if (isCommand) {
        funcAsm.push_back(asmOp("j", {LABEL_CMD_LOOP}));
        funcAsm.push_back(asmNOP());
      } else {
        funcAsm.push_back(asmOp("jr", {reg::Reg::RA}));
        funcAsm.push_back(asmNOP());
      }
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

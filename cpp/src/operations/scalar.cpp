#include "scalar.h"

#include "../asm.h"
#include "../registers.h"
#include "../state.h"
#include "../swizzle.h"
#include "../types.h"

#include <cmath>
#include <cstdint>
#include <string>

namespace rspl::ops {

static void assertScalarVars(const VarDef &varLeft,
                             const VarDef *varRight = nullptr) {
  // Memory labels (empty reg) are always treated as scalar addresses
  if ((isVecType(varLeft.type) && !varLeft.reg.starts_with("%lo")) ||
      (varRight && !varRight->reg.empty() &&
       !varRight->reg.starts_with("%lo") &&
       isVecType(varRight->type))) {
    state.throwError(
        "Scalar-Operation requires all variables to be scalars!");
  }
}

// Precompute mul->shift table
static int mulToShift(int64_t val) {
  for (int i = 0; i < 32; i++) {
    if (static_cast<int64_t>(1LL << i) == val) return i;
  }
  return -1;
}

std::vector<AsmInst> loadImmediate(const std::string &regDst,
                                   const std::string &valueStr) {
  // Labels are always ≤16 bit, use ori
  if (!valueStr.empty() && valueStr[0] == '%') {
    return {asmOp("ori", {regDst, "$zero", valueStr})};
  }

  int64_t signedVal;
  try {
    signedVal = std::stoll(valueStr);
  } catch (...) {
    // Non-numeric strings treated as labels
    return {asmOp("ori", {regDst, "$zero", valueStr})};
  }
  uint32_t valueU32 = static_cast<uint32_t>(signedVal);

  if (valueU32 == 0) {
    return {asmOp("or", {regDst, "$zero", "$zero"})};
  }
  if (u32InS16Range(valueU32)) {
    int16_t valS16 = static_cast<int16_t>(valueU32 & 0xFFFF);
    return {asmOp("addiu", {regDst, "$zero", std::to_string(valS16)})};
  }
  if (valueU32 <= 0xFFFF) {
    return {asmOp("ori", {regDst, "$zero", toHex(valueU32)})};
  }
  if ((valueU32 & 0xFFFF) == 0) {
    return {asmOp("lui", {regDst, toHex(valueU32 >> 16)})};
  }
  return {asmOp("lui", {regDst, toHex(valueU32 >> 16)}),
          asmOp("ori", {regDst, regDst, toHex(valueU32 & 0xFFFF)})};
}

std::vector<AsmInst> opMove(const VarDef &varRes,
                            const VarDef &varRight) {
  if (!varRes.swizzle.empty())
    state.throwError("Swizzling not allowed on scalar variables!");

  if (!varRight.reg.empty()) {
    if (!varRight.swizzle.empty() && !isVecType(varRight.type)) {
      state.throwError("Swizzling not allowed for scalar operations!");
    }
    if (varRes.reg == varRight.reg) {
      state.logWarning("Self-assignment detected, this is a NOP!", {});
      return {};
    }
    if (!varRight.swizzle.empty()) {
      auto sit = SWIZZLE_MAP.find(varRight.swizzle);
      if (sit == SWIZZLE_MAP.end())
        state.throwError("Unknown swizzle: " + varRight.swizzle);
      if (varRight.type == "vec16") {
        return {asmOp("mfc2",
                      {varRes.reg, varRight.reg + sit->second})};
      }
      return {asmOp("mfc2", {varRes.reg, varRight.reg + sit->second}),
              asmOp("andi", {varRes.reg, varRes.reg, "0xFFFF"}),
              asmOp("mfc2", {"$at", varRight.reg + sit->second}),
              asmOp("sll", {"$at", "$at", "16"}),
              asmOp("or", {varRes.reg, varRes.reg, "$at"})};
    }
    // Label references should go through loadImmediate for proper op selection
    if (!varRight.reg.empty() && varRight.reg[0] == '%') {
      return loadImmediate(varRes.reg, varRight.reg);
    }
    return {asmOp("or", {varRes.reg, "$zero", varRight.reg})};
  }
  // varRight is a constant
  return loadImmediate(varRes.reg, varRight.reg.empty()
                                       ? std::to_string(static_cast<int64_t>(varRight.value))
                                       : varRight.reg);
}

std::vector<AsmInst> opLoad(const VarDef &varRes, const VarOrMem &varLoc,
                            const VarOrMem &varOffset) {
  // Map type to load opcode
  static const std::unordered_map<std::string, std::string> loadMap = {
      {"u8", "lbu"}, {"s8", "lb"},  {"u16", "lhu"},
      {"s16", "lh"}, {"u32", "lw"}, {"s32", "lw"},
  };
  auto it = loadMap.find(varRes.type);
  std::string opName = it != loadMap.end() ? it->second : "lw";

  // Build the offset string
  std::string offsetStr;
  if (!varOffset.reg.empty() && varOffset.reg[0] != '%') {
    offsetStr = varOffset.reg;
  } else if (!varOffset.name.empty() && !varOffset.reg.empty() && varOffset.reg[0] == '%') {
    offsetStr = varOffset.reg; // already formatted like %lo(NAME)
  } else if (!varOffset.name.empty()) {
    offsetStr = "%lo(" + varOffset.name + ")";
  } else if (varOffset.reg.empty()) {
    offsetStr = "0";
  } else {
    offsetStr = varOffset.reg;
  }

  if (!varLoc.reg.empty()) {
    return {asmOp(opName, {varRes.reg, offsetStr + "(" + varLoc.reg + ")"})};
  }
  // Memory label base — use %lo() with offset
  return {asmOp(opName, {varRes.reg,
                         "%lo(" + varLoc.name + " + " + offsetStr + ")"})};
}

std::vector<AsmInst> opStore(const VarDef &varRes,
                             const std::vector<VarOrMem> &varOffsets) {
  if (varOffsets.empty())
    state.throwError("Store needs at least one offset argument!");

  const auto &varLoc = varOffsets[0];
  static const std::unordered_map<std::string, std::string> storeMap = {
      {"u8", "sb"},  {"s8", "sb"},  {"u16", "sh"},
      {"s16", "sh"}, {"u32", "sw"}, {"s32", "sw"},
  };
  auto it = storeMap.find(varRes.type);
  std::string opName = it != storeMap.end() ? it->second : "sw";

  std::string baseReg = varLoc.reg.empty() ? "$zero" : varLoc.reg;

  // Collect offset parts
  std::vector<std::string> offsetParts;
  bool allNumeric = true;
  for (size_t i = 1; i < varOffsets.size(); ++i) {
    if (!varOffsets[i].reg.empty() && varOffsets[i].reg[0] != '%') {
      offsetParts.push_back(varOffsets[i].reg);
      allNumeric = allNumeric && varOffsets[i].name.empty();
    } else if (!varOffsets[i].name.empty()) {
      offsetParts.push_back(varOffsets[i].name);
      allNumeric = false;
    } else if (!varOffsets[i].reg.empty()) {
      offsetParts.push_back(varOffsets[i].reg);
      allNumeric = false;
    }
  }
  // If base is a memory label, push it as an extra offset
  if (varLoc.reg.empty() && !varLoc.name.empty()) {
    offsetParts.push_back(varLoc.name);
    allNumeric = false;
  }

  std::string offset;
  for (size_t i = 0; i < offsetParts.size(); ++i) {
    if (i > 0) offset += " + ";
    offset += offsetParts[i];
  }
  if (!offset.empty() && !allNumeric) {
    offset = "%lo(" + offset + ")";
  }

  return {asmOp(opName,
                {varRes.reg, offset + "(" + baseReg + ")"})};
}

// Generic reg-or-immediate operation
static std::vector<AsmInst>
opRegOrImmediate(const std::string &opReg, const std::string &opImm,
                 bool (*rangeCheck)(uint32_t), const VarDef &varRes,
                 const VarDef &varLeft, const VarDef &varRight) {
  assertScalarVars(varLeft, &varRight);
  if (!varRight.reg.empty()) {
    // Label references like %lo(NAME) should use immediate form
    if (varRight.reg[0] == '%') {
      return {asmOp(opImm, {varRes.reg, varLeft.reg, varRight.reg})};
    }
    return {asmOp(opReg, {varRes.reg, varLeft.reg, varRight.reg})};
  }
  uint32_t valU32 = varRight.value;
  if (rangeCheck(valU32)) {
    return {asmOp(opImm, {varRes.reg, varLeft.reg,
                          std::to_string(valU32 & 0xFFFF)})};
  }
  auto load = loadImmediate("$at", std::to_string(valU32));
  load.push_back(asmOp(opReg, {varRes.reg, varLeft.reg, "$at"}));
  return load;
}

std::vector<AsmInst> opAdd(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight) {
  assertScalarVars(varLeft, &varRight);
  return opRegOrImmediate("addu", "addiu", u32InS16Range, varRes, varLeft,
                          varRight);
}

std::vector<AsmInst> opSub(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight) {
  assertScalarVars(varLeft, &varRight);
  if (!varRight.reg.empty()) {
    if (varRight.reg[0] == '%') {
      state.throwError("Subtraction cannot use labels!");
    }
    return {asmOp("subu", {varRes.reg, varLeft.reg, varRight.reg})};
  }
  VarDef negRight = varRight;
  negRight.value = -varRight.value;
  return opAdd(varRes, varLeft, negRight);
}

std::vector<AsmInst> opMul(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight, bool /*clearAccum*/) {
  assertScalarVars(varLeft, &varRight);
  int shift = mulToShift(varRight.value);
  if (!varRight.reg.empty() || shift < 0) {
    state.throwError(
        "Scalar-Multiplication only allowed with a power-of-two constant!");
  }
  if (varRight.value == 1) {
    state.throwError("Scalar-Multiplication with 1 is a NOP!");
  }
  VarDef shiftVar = varRight;
  shiftVar.value = shift;
  return opShiftLeft(varRes, varLeft, shiftVar);
}

std::vector<AsmInst> opDiv(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight) {
  assertScalarVars(varLeft, &varRight);
  int shift = mulToShift(varRight.value);
  if (!varRight.reg.empty() || shift < 0) {
    state.throwError(
        "Scalar-Division only allowed with a power-of-two constant!");
  }
  if (varRight.value == 1) {
    state.throwError("Scalar-Division by 1 is a NOP!");
  }
  VarDef shiftVar = varRight;
  shiftVar.value = shift;
  bool logical = !varLeft.type.empty() && varLeft.type[0] == 'u';
  return opShiftRight(varRes, varLeft, shiftVar, logical);
}

std::vector<AsmInst> opShiftLeft(const VarDef &varRes,
                                 const VarDef &varLeft,
                                 const VarDef &varRight) {
  assertScalarVars(varLeft, &varRight);
  if (varRight.value < 0 || varRight.value > 31) {
    state.throwError("Shift-Left value must be in range 0<x<32!");
  }
  if (!varRight.reg.empty()) {
    return {asmOp("sllv",
                  {varRes.reg, varLeft.reg, varRight.reg})};
  }
  return {asmOp("sll", {varRes.reg, varLeft.reg,
                        std::to_string(static_cast<int>(varRight.value))})};
}

std::vector<AsmInst> opShiftRight(const VarDef &varRes,
                                  const VarDef &varLeft,
                                  const VarDef &varRight, bool logical) {
  assertScalarVars(varLeft, &varRight);
  if (varRight.value < 0 || varRight.value > 31) {
    state.throwError("Shift-Right value must be in range 0<x<32!");
  }
  bool useLogical = logical || !isSigned(varRes.type);
  std::string instr = useLogical ? "srl" : "sra";
  if (!varRight.reg.empty()) instr += "v";
  std::string valR =
      varRight.reg.empty() ? std::to_string(static_cast<int>(varRight.value)) : varRight.reg;
  return {asmOp(instr, {varRes.reg, varLeft.reg, valR})};
}

std::vector<AsmInst> opAnd(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight) {
  return opRegOrImmediate("and", "andi", u32InU16Range, varRes, varLeft,
                          varRight);
}

std::vector<AsmInst> opOr(const VarDef &varRes, const VarDef &varLeft,
                          const VarDef &varRight) {
  return opRegOrImmediate("or", "ori", u32InU16Range, varRes, varLeft,
                          varRight);
}

std::vector<AsmInst> opXOR(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight) {
  return opRegOrImmediate("xor", "xori", u32InU16Range, varRes, varLeft,
                          varRight);
}

std::vector<AsmInst> opNOR(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight) {
  if (varRight.reg.empty())
    state.throwError("NOR is only supported for variables!");
  return opRegOrImmediate("nor", "nori", u32InU16Range, varRes, varLeft,
                          varRight);
}

std::vector<AsmInst> opBitFlip(const VarDef &varRes,
                               const VarDef &varRight) {
  if (varRight.reg.empty())
    state.throwError("Bitflip is only supported for variables!");
  return {asmOp("nor", {varRes.reg, "$zero", varRight.reg})};
}

std::vector<AsmInst> opCompare(const VarDef &varRes, const VarDef &varLeft,
                               const VarDef &varRight,
                               const std::string &op, bool /*ternary*/) {
  if (op == "<") {
    return {asmOp("slt", {varRes.reg, varLeft.reg, varRight.reg})};
  }
  if (op == ">") {
    return {asmOp("slt", {varRes.reg, varRight.reg, varLeft.reg})};
  }
  state.throwError("Compare op '" + op + "' not implemented yet!");
  return {};
}

} // namespace rspl::ops

// TEMPORARY DEBUG
#include <iostream>
static int debugStoreCount = 0;

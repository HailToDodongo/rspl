#include "vector.h"
#include "scalar.h"

#include "../asm.h"
#include "../registers.h"
#include "../state.h"
#include "../swizzle.h"
#include "../types.h"

#include <string>
#include <unordered_set>

namespace rspl::ops {

// --- Vec32 register helpers (mirror registers.js) ---------------------

static const std::string *nextVecReg(const std::string &regName) {
  return reg::nextVecReg(regName);
}

static std::string intReg(const VarDef &v) { return v.reg; }

static std::string fractReg(const VarDef &v) {
  if (v.type == "vec32" || v.originalType == "vec32") {
    const auto *next = reg::nextVecReg(v.reg);
    return next ? *next : reg::Reg::VZERO;
  }
  if (v.castType == "ufract" || v.castType == "sfract") {
    return v.reg;
  }
  return reg::Reg::VZERO;
}

// Returns the two registers that represent the full vec32 value.
// For sources with a cast (ufract/sfract), the irrelevant half is VZERO.
// For destinations, callers should use getVec32DstRegs() to always get the
// original register pair.
std::pair<std::string, std::string> getVec32Regs(const VarDef &v) {
  if (v.type == "vec32") {
    const auto *next = reg::nextVecReg(v.reg);
    return {v.reg, next ? *next : reg::Reg::VZERO};
  }
  if (v.castType == "ufract" || v.castType == "sfract") {
    return {reg::Reg::VZERO, v.reg};
  }
  return {v.reg, reg::Reg::VZERO};
}

// JS getVec32RegsResLR: when the result is not a two-reg type (cast or
// vec16), the meaningful source register is used for both halves of the
// source pair so vmrg writes land correctly.
static void adjustRegsForResType(const VarDef &varRes, const VarDef &varLeft,
                                  const VarDef &varRight,
                                  std::pair<std::string, std::string> &regsL,
                                  std::pair<std::string, std::string> &regsR) {
  if (!isTwoRegType(varRes.type)) {
    regsL.first = varLeft.reg;
    regsR.first = varRight.reg;
  }
}

// Returns the actual register pair of the original vec32, regardless of cast.
static std::pair<std::string, std::string>
getVec32DstRegs(const VarDef &v) {
  if (v.type == "vec32") {
    const auto *next = reg::nextVecReg(v.reg);
    return {v.reg, next ? *next : reg::Reg::VZERO};
  }
  if (v.originalType == "vec32") {
    if (v.castType == "ufract" || v.castType == "sfract") {
      // v.reg is the fract register; the int reg is the previous one
      const auto *prev = reg::nextReg(v.reg, -1);
      std::string intReg = prev ? *prev : reg::Reg::VZERO;
      return {intReg, v.reg};
    }
    // sint/uint: v.reg is the int register
    const auto *next = reg::nextVecReg(v.reg);
    return {v.reg, next ? *next : reg::Reg::VZERO};
  }
  // Shouldn't be reached for vec32 destinations
  return {v.reg, reg::Reg::VZERO};
}

static void assertVectorVars(const VarDef &varLeft,
                             const VarDef *varRight = nullptr) {
  if (!isVecType(varLeft.type) ||
      (varRight && !varRight->reg.empty() &&
       !isVecType(varRight->type))) {
    state.throwError(
        "Vector-Operation requires all variables to be vectors!");
  }
}

// --- Generic logic op (vand, vor, vxor, vnor) -------------------------

static std::vector<AsmInst>
genericLogicOp(const VarDef &varRes, const VarDef &varLeft,
               const VarDef &varRight, const std::string &op) {
  std::string funcName;
  for (char c : op)
    funcName += static_cast<char>(std::toupper(c));
  if (varRight.reg.empty())
    state.throwError(funcName + " cannot be done with a constant!");
  if (!varRes.swizzle.empty() || !varLeft.swizzle.empty())
    state.throwError(funcName +
                     " only allows swizzle on the right side!");
  assertVectorVars(varLeft, &varRight);

  auto sit = SWIZZLE_MAP.find(varRight.swizzle);
  if (sit == SWIZZLE_MAP.end())
    state.throwError("Unsupported swizzle: " + varRight.swizzle);

  bool is32 = (varRes.type == "vec32");
  std::string swSuffix = sit->second;
  std::string regR = varRight.reg + swSuffix;

  std::vector<AsmInst> res;
  res.push_back(asmOp(op, {varRes.reg, varLeft.reg, regR}));
  if (is32) {
    res.push_back(asmOp(op, {*reg::nextVecReg(varRes.reg),
                             fractReg(varLeft), fractReg(varRight) +
                                                    swSuffix}));
  }
  return res;
}

// --- opMoveVec --------------------------------------------------------

std::vector<AsmInst> opMoveVec(const VarDef &varRes,
                               const VarDef &varRight) {
  bool isVec32 = (varRes.type == "vec32" ||
                   varRes.originalType == "vec32");

  // Constant assignment to full vector
  if (varRight.reg.empty() && varRes.swizzle.empty()) {
    auto pIt = POW2_SWIZZLE_VAR.find(varRight.value);
    if (pIt == POW2_SWIZZLE_VAR.end()) {
      state.throwError(
          "Can only assign a constant to a vector if it is a power of "
          "two or zero!");
    }
    auto sit = SWIZZLE_MAP.find(pIt->second.swizzle);
    std::string regPow = pIt->second.reg + sit->second;
    std::vector<AsmInst> res;
    auto regsDst = getVec32Regs(varRes);
    bool hasCast =
        !varRes.castType.empty() || !varRes.swizzle.empty();
    if (hasCast) {
      // For casts, only the relevant half gets the constant
      if (regsDst.second != reg::Reg::VZERO)
        res.push_back(
            asmOp("vxor", {regsDst.second, reg::Reg::VZERO, regPow}));
    } else {
      res.push_back(
          asmOp("vxor", {regsDst.first, reg::Reg::VZERO, regPow}));
      if (isVec32 && regsDst.second != reg::Reg::VZERO)
        res.push_back(
            asmOp("vxor", {regsDst.second, reg::Reg::VZERO,
                            reg::Reg::VZERO}));
    }
    return res;
  }

  // Scalar -> vector
  bool isScalar = !varRight.reg.empty() && !isVecType(varRight.type) &&
                  varRight.reg[0] != '%';
  if (isScalar) {
    auto swizzleRes = SWIZZLE_MAP.find(varRes.swizzle);
    std::string sRes =
        swizzleRes != SWIZZLE_MAP.end() ? swizzleRes->second : "";
    bool needsExpand = varRes.swizzle.empty();
    if (needsExpand)
      sRes = ".e0";

    std::vector<AsmInst> scalarRes;
    if (isVec32 || varRes.originalType == "vec32") {
      auto regsDst = getVec32DstRegs(varRes);
      scalarRes.push_back(
          asmOp("mtc2", {varRight.reg, regsDst.second + sRes}));
      scalarRes.push_back(
          asmOp("srl", {reg::Reg::AT, varRight.reg, "16"}));
      scalarRes.push_back(
          asmOp("mtc2", {reg::Reg::AT, regsDst.first + sRes}));
      if (needsExpand) {
        scalarRes.push_back(
            asmOp("vor", {regsDst.first, reg::Reg::VZERO,
                          regsDst.first + sRes}));
        scalarRes.push_back(
            asmOp("vor", {regsDst.second, reg::Reg::VZERO,
                          regsDst.second + sRes}));
      }
    } else {
      scalarRes.push_back(
          asmOp("mtc2", {varRight.reg, varRes.reg + sRes}));
      if (needsExpand) {
        scalarRes.push_back(
            asmOp("vor", {varRes.reg, reg::Reg::VZERO,
                          varRes.reg + sRes}));
      }
    }
    return scalarRes;
  }

  // Vector -> vector
  if (!varRight.reg.empty()) {
    auto regsDst = getVec32DstRegs(varRes);
    auto regsR = getVec32Regs(varRight);

    // When both sides have casts, only use the base register of each
    // (the paired register is VZERO, whose writes are filtered as NOPs).
    if (!varRes.castType.empty() && !varRight.castType.empty()) {
      regsDst = {varRes.reg, reg::Reg::VZERO};
      regsR = {varRight.reg, reg::Reg::VZERO};
    }

    // Full vector move
    if (varRight.swizzle.empty()) {
      std::vector<AsmInst> moveRes;
      moveRes.push_back(
          asmOp("vor", {regsDst.first, reg::Reg::VZERO, regsR.first}));
      if (isVec32) {
        moveRes.push_back(
            asmOp("vor", {regsDst.second, reg::Reg::VZERO, regsR.second}));
      }
      return moveRes;
    }

    // Broadcast swizzle into full vector
    if (varRes.swizzle.empty()) {
      auto sit = SWIZZLE_MAP.find(varRight.swizzle);
      std::string suffix =
          sit != SWIZZLE_MAP.end() ? sit->second : "";
      std::vector<AsmInst> broadcastRes;
      broadcastRes.push_back(
          asmOp("vor",
                {regsDst.first, reg::Reg::VZERO, regsR.first + suffix}));
      if (isVec32) {
        broadcastRes.push_back(asmOp("vor", {regsDst.second, reg::Reg::VZERO,
                                             regsR.second + suffix}));
      }
      return broadcastRes;
    }

    // Half-vector move: 4-lane swizzle on both sides requires
    // storing through scratch memory since vmov only moves single lanes
    // and sdv/ldv can move 8 bytes (4 lanes) at once.
    // Examples: res.xyzw = a.XYZW (upper→lower),
    //           res.XYZW = a.xyzw (lower→upper)
    bool isHalfMove =
        !varRes.swizzle.empty() && varRes.swizzle.size() == 4 &&
        !varRight.swizzle.empty() && varRight.swizzle.size() == 4;
    if (isHalfMove) {
      auto sitSrc = SWIZZLE_SCALAR_IDX.find(varRight.swizzle[0]);
      auto sitDst = SWIZZLE_SCALAR_IDX.find(varRes.swizzle[0]);
      if (sitSrc != SWIZZLE_SCALAR_IDX.end() &&
          sitDst != SWIZZLE_SCALAR_IDX.end()) {
        int srcOffset = sitSrc->second * 2; // byte offset into source reg
        int dstOffset = sitDst->second * 2; // byte offset into dest reg
        int accessLen = 8; // 4 lanes × 2 bytes = 8 bytes

        state.addAnnotation("Barrier", "__SCRATCH_MEM__");

        std::vector<AsmInst> halfRes;
        halfRes.push_back(
            asmOp("ori", {reg::Reg::AT, reg::Reg::ZERO,
                          "%lo(RSPQ_SCRATCH_MEM)"}));
        halfRes.push_back(
            asmOp("sdv", {regsR.first, std::to_string(srcOffset), "0",
                          reg::Reg::AT}));
        if (regsR.second != reg::Reg::VZERO)
          halfRes.push_back(
              asmOp("sdv", {regsR.second, std::to_string(srcOffset),
                            std::to_string(accessLen), reg::Reg::AT}));
        halfRes.push_back(
            asmOp("ldv", {regsDst.first, std::to_string(dstOffset), "0",
                          reg::Reg::AT}));
        if (regsDst.second != reg::Reg::VZERO)
          halfRes.push_back(
              asmOp("ldv", {regsDst.second, std::to_string(dstOffset),
                            std::to_string(accessLen), reg::Reg::AT}));
        return halfRes;
      }
    }

    // Single lane move
    auto sitRes = SWIZZLE_MAP.find(varRes.swizzle);
    auto sitRight = SWIZZLE_MAP.find(varRight.swizzle);
    std::string sRes =
        sitRes != SWIZZLE_MAP.end() ? sitRes->second : "";
    std::string sRight =
        sitRight != SWIZZLE_MAP.end() ? sitRight->second : "";
    std::vector<AsmInst> laneRes;
    laneRes.push_back(
        asmOp("vmov", {regsDst.first + sRes, regsR.first + sRight}));
    if (isVec32) {
      laneRes.push_back(asmOp("vmov",
                              {regsDst.second + sRes, regsR.second + sRight}));
    }
    return laneRes;
  }

  // Constant to single lane or scaled assignment
  if (varRight.reg.empty()) {
    auto sit = SWIZZLE_MAP.find(varRes.swizzle);
    std::string sRes = sit != SWIZZLE_MAP.end() ? sit->second : "";
    int64_t val = varRight.value;

    // Power-of-two lookup (works for both full-vector and single-lane)
    auto pIt = POW2_SWIZZLE_VAR.find(val);
    if (pIt != POW2_SWIZZLE_VAR.end()) {
      auto swSit = SWIZZLE_MAP.find(pIt->second.swizzle);
      std::string swSuffix =
          swSit != SWIZZLE_MAP.end() ? swSit->second : "";
      if (varRes.swizzle.empty()) {
        // Full vector constant — use vxor with power-of-two reg
        return {
            asmOp("vxor", {varRes.reg, reg::Reg::VZERO,
                           pIt->second.reg + swSuffix})};
      }
      // Single lane — use vmov from power-of-two reg
      std::vector<AsmInst> pow2Res;
      auto regsDst = getVec32Regs(varRes);
      pow2Res.push_back(
          asmOp("vmov", {regsDst.first + sRes, pIt->second.reg + swSuffix}));
      if (isVec32) {
        auto zeroIt = POW2_SWIZZLE_VAR.find(0);
        auto zSwSit =
            zeroIt != POW2_SWIZZLE_VAR.end()
                ? SWIZZLE_MAP.find(zeroIt->second.swizzle)
                : SWIZZLE_MAP.end();
        std::string zSuffix =
            zSwSit != SWIZZLE_MAP.end() ? zSwSit->second : "";
        pow2Res.push_back(
            asmOp("vmov", {regsDst.second + sRes,
                            zeroIt->second.reg + zSuffix}));
      }
      return pow2Res;
    }

    // Full vector constant (non-power-of-two) — try vxor for zero
    if (varRes.swizzle.empty() && val == 0) {
      auto zeroIt = POW2_SWIZZLE_VAR.find(0);
      auto zSit = SWIZZLE_MAP.find(zeroIt->second.swizzle);
      return {asmOp("vxor", {varRes.reg, reg::Reg::VZERO,
                             zeroIt->second.reg + zSit->second})};
    }

    // Float constant — convert to FP32->FP16 fixed-point parts
    double dVal = varRight.value;
    if (dVal != static_cast<int64_t>(dVal)) {
      bool isFractCast =
          varRes.castType == "ufract" || varRes.castType == "sfract";
      double scale = (varRes.castType == "sfract") ? 0.5 : 1.0;
      auto valueFP32 =
          static_cast<int64_t>(dVal * scale * 65536.0);
      int64_t valInt = (valueFP32 >> 16) & 0xFFFF;
      int64_t valFract = valueFP32 & 0xFFFF;
      if (varRes.castType == "sfract" && dVal >= 0) {
        valFract = std::min(valFract, int64_t(0x7FFF));
      }
      if (isFractCast) valInt = valFract;

      std::vector<AsmInst> fload;
      auto regsDst = getVec32Regs(varRes);
      if (valInt != 0) {
        auto li = loadImmediate(reg::Reg::AT, std::to_string(valInt));
        fload.insert(fload.end(), li.begin(), li.end());
      }
      fload.push_back(
          asmOp("mtc2", {valInt == 0 ? reg::Reg::ZERO : reg::Reg::AT,
                          regsDst.first + sRes}));
      if (isVec32 || varRes.originalType == "vec32") {
        if (valFract != 0) {
          auto li =
              loadImmediate(reg::Reg::AT, std::to_string(valFract));
          fload.insert(fload.end(), li.begin(), li.end());
        }
        fload.push_back(
            asmOp("mtc2", {valFract == 0 ? reg::Reg::ZERO : reg::Reg::AT,
                            regsDst.second + sRes}));
      } else if (isFractCast) {
        // For fract cast on vec16, only the fract value is used (already in
        // valInt)
      }
      return fload;
    }

    // Load immediate into $at and use mtc2
    auto load =
        ops::loadImmediate(reg::Reg::AT, std::to_string(val));
    load.push_back(
        asmOp("mtc2", {reg::Reg::AT, varRes.reg + sRes}));
    // Expand to full vector if no swizzle
    if (varRes.swizzle.empty()) {
      load.push_back(
          asmOp("vor", {varRes.reg, reg::Reg::VZERO,
                        varRes.reg + sRes}));
    }
    return load;
  }

  state.throwError("Unhandled vector move case");
  return {};
}

// --- opLoadVec --------------------------------------------------------

std::vector<AsmInst> opLoadVec(const VarDef &varRes,
                               const VarOrMem &varLoc,
                               const VarOrMem &varOffset,
                               const std::string &swizzle,
                               bool isPackedByte, bool isSigned,
                               bool isUnaligned) {
  std::vector<AsmInst> res;
  VarOrMem loc = varLoc;
  VarOrMem offs = varOffset;

  // Resolve label address into $at
  if (loc.reg.empty() && !loc.name.empty()) {
    auto load = loadImmediate(reg::Reg::AT, "%lo(" + loc.name + ")");
    res.insert(res.end(), load.begin(), load.end());
    loc.reg = reg::Reg::AT;
  }

  // Compute destination offset from result swizzle
  int destOffset = 0;
  if (!varRes.swizzle.empty()) {
    auto sit = SWIZZLE_SCALAR_IDX.find(varRes.swizzle[0]);
    if (sit != SWIZZLE_SCALAR_IDX.end()) destOffset = sit->second * 2;
  }

  bool is32 = (varRes.type == "vec32");

  // Detect dupeLoad and normalize swizzle (matches JS behavior)
  std::string swiz = swizzle;
  bool dupeLoad = (swiz == "xyzwxyzw");
  if (dupeLoad) swiz = "xyzw";

  int accessLen = swiz.empty() ? 16 : static_cast<int>(swiz.size()) * 2;

  // Select load instruction based on access length
  static const std::unordered_map<int, std::string> loadInstrMap = {
      {2, "lsv"}, {4, "llv"}, {8, "ldv"}, {16, "lqv"},
  };
  auto lit = loadInstrMap.find(accessLen);
  if (lit == loadInstrMap.end()) {
    state.throwError("Invalid load access length");
    return {};
  }
  std::string loadInstr = lit->second;

  // Packed byte overrides instruction
  (void)isSigned;
  if (isPackedByte) {
    if (is32) state.throwError("Packed byte loads are not supported for 32-bit vectors!");
    loadInstr = isSigned ? "lpv" : "luv";
    destOffset /= 2;
  }

  // Compute source offset from swizzle + numeric offset
  int srcOffset = 0;
  if (!swiz.empty()) {
    auto ssi = SWIZZLE_SCALAR_IDX.find(swiz[0]);
    if (ssi != SWIZZLE_SCALAR_IDX.end()) srcOffset = ssi->second * 2;
  }
  if (!offs.reg.empty()) srcOffset += static_cast<int>(std::stoll(offs.reg));

  // Alignment check for full vector loads
  if (loadInstr == "lqv" && (srcOffset % 16) != 0) {
    state.throwError("Invalid full vector-load offset, must be a multiple of 16, " + std::to_string(srcOffset) + " given");
  }

  std::string alignOp;
  if (isUnaligned && loadInstr == "lqv") alignOp = "lrv";

  // Emit load instruction(s)
  auto emit = [&](const std::string &reg, int dOff, int sOff) {
    res.push_back(asmOp(loadInstr, {reg, std::to_string(dOff), std::to_string(sOff), loc.reg}));
  };

  emit(varRes.reg, destOffset, srcOffset);
  if (dupeLoad) emit(varRes.reg, destOffset + 8, srcOffset);

  if (!alignOp.empty()) {
    auto emitA = [&](const std::string &reg, int dOff, int sOff) {
      res.push_back(asmOp(alignOp, {reg, std::to_string(dOff), std::to_string(sOff), loc.reg}));
    };
    emitA(varRes.reg, destOffset, srcOffset + 0x10);
    if (dupeLoad) emitA(varRes.reg, destOffset + 8, srcOffset + 0x10);
  }

  if (is32) {
    const auto *nextRegV = reg::nextVecReg(varRes.reg);
    if (nextRegV) {
      emit(*nextRegV, destOffset, srcOffset + accessLen);
      if (dupeLoad) emit(*nextRegV, destOffset + 8, srcOffset + accessLen);
      if (!alignOp.empty()) {
        auto emitA = [&](const std::string &reg, int dOff, int sOff) {
          res.push_back(asmOp(alignOp, {reg, std::to_string(dOff), std::to_string(sOff), loc.reg}));
        };
        emitA(*nextRegV, destOffset, srcOffset + accessLen + 0x10);
        if (dupeLoad) emitA(*nextRegV, destOffset + 8, srcOffset + accessLen + 0x10);
      }
    }
  }

  return res;
}

std::vector<AsmInst> opLoadBytes(const VarDef &varRes,
                                 const VarOrMem &varLoc,
                                 const VarOrMem &varOffset,
                                 const std::string &swizzle,
                                 bool isSigned) {
  return opLoadVec(varRes, varLoc, varOffset, swizzle, true, isSigned,
                   false);
}

// --- opStoreVec -------------------------------------------------------

std::vector<AsmInst> opStoreVec(const VarDef &varRes,
                                const std::vector<VarOrMem> &varOffsets,
                                bool isPackedByte, bool isSigned,
                                bool isUnaligned) {
  if (varOffsets.empty())
    state.throwError("Vector stores need at least one offset!");
  const auto &varLoc = varOffsets[0];

  bool is32 = (varRes.type == "vec32");
  int accessLen = varRes.swizzle.empty()
                      ? 16
                      : static_cast<int>(varRes.swizzle.size()) * 2;

  std::string storeInstr;
  switch (accessLen) {
    case 2: storeInstr = "ssv"; break;
    case 4: storeInstr = "slv"; break;
    case 8: storeInstr = "sdv"; break;
    case 16: storeInstr = "sqv"; break;
    default: state.throwError("Invalid store access length"); return {};
  }

  int srcOffset = varRes.swizzle.empty()
                      ? 0
                      : (SWIZZLE_SCALAR_IDX.count(varRes.swizzle[0])
                             ? SWIZZLE_SCALAR_IDX.at(varRes.swizzle[0]) * 2
                             : 0);

  if (isPackedByte) {
    if (is32) state.throwError("Packed byte stores are not supported for 32-bit vectors!");
    if (!varRes.swizzle.empty() && varRes.swizzle.size() != 1)
      state.throwError("Packed byte stores only support single-lane swizzles!");
    storeInstr = isSigned ? "spv" : "suv";
    srcOffset /= 2;
  }

  std::string alignOp;
  if (isUnaligned && storeInstr == "sqv") alignOp = "srv";

  std::vector<AsmInst> res;

  std::string baseReg = varLoc.reg;
  if (varLoc.reg.empty() && !varLoc.name.empty()) {
    auto load = loadImmediate(reg::Reg::AT, "%lo(" + varLoc.name + ")");
    res.insert(res.end(), load.begin(), load.end());
    baseReg = reg::Reg::AT;
  }

  // Sum all offset arguments (matching JS opStore.js:280-284)
  int baseOffset = 0;
  for (size_t i = 1; i < varOffsets.size(); ++i) {
    if (!varOffsets[i].reg.empty())
      baseOffset += std::stoi(varOffsets[i].reg);
  }

  auto emit = [&](const std::string &reg, int sOff, int bOff) {
    res.push_back(asmOp(storeInstr, {reg, std::to_string(sOff), std::to_string(bOff), baseReg}));
  };
  auto emitA = [&](const std::string &reg, int sOff, int bOff) {
    res.push_back(asmOp(alignOp, {reg, std::to_string(sOff), std::to_string(bOff), baseReg}));
  };

  emit(varRes.reg, srcOffset, baseOffset);
  if (!alignOp.empty()) emitA(varRes.reg, srcOffset, baseOffset + 0x10);

  if (is32) {
    const auto *nextRegV = reg::nextVecReg(varRes.reg);
    if (nextRegV) {
      emit(*nextRegV, srcOffset, baseOffset + accessLen);
      if (!alignOp.empty()) emitA(*nextRegV, srcOffset, baseOffset + accessLen + 0x10);
    }
  }

  return res;
}

std::vector<AsmInst> opStoreBytes(const VarDef &varRes,
                                  const std::vector<VarOrMem> &varOffsets,
                                  bool isSigned) {
  return opStoreVec(varRes, varOffsets, true, isSigned, false);
}

// --- Arithmetic -------------------------------------------------------

std::vector<AsmInst> opAddVec(const VarDef &varRes,
                              const VarDef &varLeft,
                              VarDef varRight) {
  if (varRight.reg.empty()) {
    auto pIt = POW2_SWIZZLE_VAR.find(varRight.value);
    if (pIt == POW2_SWIZZLE_VAR.end()) {
      state.throwError("Addition by a constant can only be done with "
                       "powers of two!");
    }
    varRight.reg = pIt->second.reg;
    varRight.swizzle = pIt->second.swizzle;
  }
  if (!varRes.swizzle.empty() || !varLeft.swizzle.empty()) {
    state.throwError("Addition only allows swizzle on the right side!");
  }
  assertVectorVars(varLeft, &varRight);

  auto sit = SWIZZLE_MAP.find(varRight.swizzle);
  if (sit == SWIZZLE_MAP.end())
    state.throwError("Unsupported swizzle: " + varRight.swizzle);

  auto regsDst = getVec32Regs(varRes);
  auto regsL = getVec32Regs(varLeft);
  auto regsR = getVec32Regs(varRight);

  std::string fractOp = "vaddc";
  std::string intOp = "vaddc";
  if (varRes.type == "vec32") {
    fractOp = "vaddc";
    intOp = "vadd";
  } else if (!varRes.castType.empty()) {
    if (varRes.castType == "sfract") fractOp = "vadd";
    if (varRes.castType == "sint") intOp = "vadd";
  }

  return {asmOp(fractOp, {regsDst.second, regsL.second,
                          regsR.second + sit->second}),
          asmOp(intOp, {regsDst.first, regsL.first,
                        regsR.first + sit->second})};
}

std::vector<AsmInst> opSubVec(const VarDef &varRes,
                              const VarDef &varLeft,
                              VarDef varRight) {
  if (varRight.reg.empty()) {
    auto pIt = POW2_SWIZZLE_VAR.find(varRight.value);
    if (pIt == POW2_SWIZZLE_VAR.end()) {
      state.throwError(
          "Subtraction by a constant can only be done with powers of "
          "two!");
    }
    varRight.reg = pIt->second.reg;
    varRight.swizzle = pIt->second.swizzle;
  }
  assertVectorVars(varLeft, &varRight);
  auto sit = SWIZZLE_MAP.find(varRight.swizzle);
  if (sit == SWIZZLE_MAP.end())
    state.throwError("Unsupported swizzle: " + varRight.swizzle);

  if (varRes.type == "vec32") {
    return {asmOp("vsubc", {*reg::nextReg(varRes.reg),
                            fractReg(varLeft),
                            fractReg(varRight) + sit->second}),
            asmOp("vsub", {varRes.reg, varLeft.reg,
                           varRight.reg + sit->second})};
  }
  bool isSigned =
      !varRes.castType.empty() && varRes.castType[0] == 's';
  return {asmOp(isSigned ? "vsub" : "vsubc",
                {varRes.reg, varLeft.reg,
                 varRight.reg + sit->second})};
}

std::vector<AsmInst> opMulVec(const VarDef &varRes,
                              const VarDef &varLeft,
                              VarDef varRight, bool clearAccum) {
  if (varRight.reg.empty()) {
    auto pIt = POW2_SWIZZLE_VAR.find(varRight.value);
    if (pIt == POW2_SWIZZLE_VAR.end()) {
      state.throwError("Multiplication by a constant can only be done "
                       "with powers of two!");
    }
    varRight.reg = pIt->second.reg;
    varRight.swizzle = pIt->second.swizzle;
    varRight.type = "vec16";
  }
  assertVectorVars(varLeft, &varRight);
  auto sit = SWIZZLE_MAP.find(varRight.swizzle);
  if (sit == SWIZZLE_MAP.end())
    state.throwError("Unsupported swizzle: " + varRight.swizzle);

  std::string swSuffix = sit->second;
  bool right32Bit = (varRight.type == "vec32");
  std::string fractOp = clearAccum ? "vmudl" : "vmadl";
  std::string intOp = clearAccum ? "vmudn" : "vmadn";

  // 16-bit multiply with cast
  // JS opMul:603-608 — vec16 result special case for sfract/ufract.
  // Unlike C++ this does NOT contain caseRef/sint/default paths;
  // non-matching cases fall through to the general multiply paths below.
  if (varRes.type == "vec16" &&
      varLeft.type == "vec16" &&
      (varLeft.castType == "sfract" || varLeft.castType == "ufract") &&
      varRight.originalType == "vec32" &&
      (varRight.castType == "sfract" || varRight.castType == "ufract")) {
    std::string opMid = clearAccum ? "vmudm" : "vmadm";
    return {asmOp(opMid, {varRes.reg, varLeft.reg,
                          varRight.reg + swSuffix})};
  }

  // vec32 sfract result from vec32 * vec32 (JS opMul:612-619)
  if (varRes.originalType == "vec32" && varRes.castType == "sfract" &&
      varLeft.type == "vec32" && varRight.type == "vec32") {
    return {
        asmOp("vmudl", {reg::Reg::VTEMP0, fractReg(varLeft),
                         fractReg(varRight) + swSuffix}),
        asmOp("vmadm", {reg::Reg::VTEMP0, intReg(varLeft),
                         fractReg(varRight) + swSuffix}),
        asmOp("vmadn", {varRes.reg, fractReg(varLeft),
                         intReg(varRight) + swSuffix}),
    };
  }

  // 16bit * 32bit multiply (JS opMul:643-649)
  if (right32Bit && varRes.type == "vec32" && varLeft.type == "vec16" &&
      !(varLeft.castType == "sfract" || varLeft.castType == "ufract")) {
    auto regsDst = getVec32Regs(varRes);
    return {
        asmOp("vmudm", {regsDst.second, varLeft.reg,
                         fractReg(varRight) + swSuffix}),
        asmOp("vmadh", {intReg(varRes), varLeft.reg,
                         intReg(varRight) + swSuffix}),
        asmOp("vmadn", {regsDst.second, reg::Reg::VZERO, reg::Reg::VZERO}),
    };
  }

  // Full 32-bit multiplication
  if (right32Bit) {
    std::vector<AsmInst> res;
    res.push_back(
        asmOp(fractOp, {reg::Reg::VTEMP0, fractReg(varLeft),
                         fractReg(varRight) + swSuffix}));
    res.push_back(
        asmOp("vmadm", {reg::Reg::VTEMP0, intReg(varLeft),
                         fractReg(varRight) + swSuffix}));
    intOp = "vmadn";
    auto regsDst = getVec32Regs(varRes);
    std::string regResFract =
        (regsDst.second == reg::Reg::VZERO) ? reg::Reg::VTEMP0
                                             : regsDst.second;
    res.push_back(
        asmOp(intOp, {regResFract, fractReg(varLeft),
                       intReg(varRight) + swSuffix}));
    res.push_back(
        asmOp("vmadh", {intReg(varRes), intReg(varLeft),
                         intReg(varRight) + swSuffix}));
    return res;
  }

  // Partial multiplication: s16.16 * 0.16 (fractional part of original s16.16)
  // JS opMul:662-668
  bool rightSideIsFraction =
      (varRight.castType == "sfract" || varRight.castType == "ufract");
  if (rightSideIsFraction &&
      (varRight.originalType == "vec32" || varRes.type == "vec32")) {
    const std::string *nextReg = reg::nextVecReg(varRes.reg);
    return {
        asmOp(fractOp,
              {*nextReg, fractReg(varLeft), varRight.reg + swSuffix}),
        asmOp("vmadm", {varRes.reg, varLeft.reg, varRight.reg + swSuffix}),
        asmOp("vmadn", {*nextReg, reg::Reg::VZERO, reg::Reg::VZERO}),
    };
  }

  // JS opMul:671-686 — vec16 result default path
  if (varRes.type == "vec16") {
    std::string caseRef =
        !varLeft.castType.empty()   ? varLeft.castType :
        !varRight.castType.empty()  ? varRight.castType :
                                      varRes.castType;
    if (caseRef == "ufract" || caseRef == "sfract") {
      std::string op = clearAccum ? "vmul" : "vmac";
      op += (caseRef == "ufract") ? "u" : "f";
      return {asmOp(op, {varRes.reg, varLeft.reg,
                         varRight.reg + swSuffix})};
    }
    if (varLeft.castType == "sint" || varRight.castType == "sint") {
      intOp = clearAccum ? "vmudh" : "vmadh";
    }
    return {asmOp(intOp, {varRes.reg, varLeft.reg,
                          varRight.reg + swSuffix})};
  }

  // JS opMul:688-692 — general vec32 path
  if (varRes.type == "vec32" || varRes.originalType == "vec32") {
    auto regsDst = getVec32Regs(varRes);
    std::string regResFract =
        (regsDst.second == reg::Reg::VZERO) ? reg::Reg::VTEMP0
                                             : regsDst.second;
    return {
        asmOp(intOp, {regResFract, fractReg(varLeft),
                       intReg(varRight) + swSuffix}),
        asmOp("vmadh", {intReg(varRes), intReg(varLeft),
                         intReg(varRight) + swSuffix})};
  }

  // 16-bit multiply (default — scalar or unhandled)
  return {asmOp(intOp, {varRes.reg, varLeft.reg,
                        varRight.reg + swSuffix})};
}

// --- Shifts -----------------------------------------------------------

std::vector<AsmInst> opShiftLeftVec(const VarDef &varRes,
                                    const VarDef &varLeft,
                                    const VarDef &varRight) {
  if (varRight.reg.empty()) {
  } else {
    state.throwError(
        "Vector-Shift amount must be a constant!");
  }
  int64_t shiftPow = 1LL << int64_t(varRight.value);
  auto pIt = POW2_SWIZZLE_VAR.find(shiftPow);
  if (pIt == POW2_SWIZZLE_VAR.end())
    state.throwError("Invalid shift value");

  auto sit = SWIZZLE_MAP.find(pIt->second.swizzle);
  std::string regR = pIt->second.reg + sit->second;

  // Vec32 shift
  if (varRes.type == "vec32") {
    auto regsDst = getVec32Regs(varRes);
    auto regsL = getVec32Regs(varLeft);
    std::string firstReg =
        (regsDst.first == regsL.first) ? reg::Reg::VTEMP0 : regsDst.first;
    return {
        asmOp("vmudl", {firstReg, regsL.second, regR}),
        asmOp("vmadn", {regsDst.first, regsL.first, regR}),
        asmOp("vmudn", {regsDst.second, regsL.second, regR})};
  }

  // Vec16 result from vec32 source
  if (varRes.type == "vec16" && varLeft.type == "vec32") {
    auto regsL = getVec32Regs(varLeft);
    return {
        asmOp("vmudl", {varRes.reg, regsL.second, regR}),
        asmOp("vmadn", {varRes.reg, regsL.first, regR})};
  }

  // Vec16 shift
  return {
      asmOp("vmudn", {varRes.reg, varLeft.reg, regR})};
}

std::vector<AsmInst> opShiftRightVec(const VarDef &varRes,
                                     const VarDef &varLeft,
                                     const VarDef &varRight,
                                     bool logical) {
  if (varRight.reg.empty()) {
  } else {
    state.throwError("Vector-Shift amount must be a constant!");
  }
  int64_t shiftVal =
      static_cast<int64_t>((1.0 / (1LL << int64_t(varRight.value))) * 0x10000);
  auto pIt = POW2_SWIZZLE_VAR.find(shiftVal);
  if (pIt == POW2_SWIZZLE_VAR.end())
    state.throwError("Invalid shift value");

  auto sit = SWIZZLE_MAP.find(pIt->second.swizzle);
  std::string regR = pIt->second.reg + sit->second;

  // Vec32 shift
  if (varRes.type == "vec32") {
    auto regsDst = getVec32Regs(varRes);
    auto regsL = getVec32Regs(varLeft);

    if (regsL.second == reg::Reg::VZERO) {
      std::string instMid = logical ? "vmudn" : "vmudm";
      return {
          asmOp(instMid, {regsDst.first, regsL.first, regR}),
          asmOp("vmadn", {regsDst.second, reg::Reg::VZERO,
                           reg::Reg::VZERO})};
    }

    std::string instMid = logical ? "vmadn" : "vmadm";
    return {
        asmOp("vmudl", {regsDst.second, regsL.second, regR}),
        asmOp(instMid, {regsDst.first, regsL.first, regR}),
        asmOp("vmadn", {regsDst.second, reg::Reg::VZERO,
                         reg::Reg::VZERO})};
  }

  // Vec16 shift
  std::string instr = logical ? "vmudl" : "vmudm";
  return {asmOp(instr, {varRes.reg, varLeft.reg, regR})};
}

// --- Bitwise ----------------------------------------------------------

std::vector<AsmInst> opAndVec(const VarDef &varRes,
                              const VarDef &varLeft,
                              const VarDef &varRight) {
  return genericLogicOp(varRes, varLeft, varRight, "vand");
}
std::vector<AsmInst> opOrVec(const VarDef &varRes,
                             const VarDef &varLeft,
                             const VarDef &varRight) {
  return genericLogicOp(varRes, varLeft, varRight, "vor");
}
std::vector<AsmInst> opNORVec(const VarDef &varRes,
                              const VarDef &varLeft,
                              const VarDef &varRight) {
  return genericLogicOp(varRes, varLeft, varRight, "vnor");
}
std::vector<AsmInst> opXORVec(const VarDef &varRes,
                              const VarDef &varLeft,
                              const VarDef &varRight) {
  return genericLogicOp(varRes, varLeft, varRight, "vxor");
}

std::vector<AsmInst> opBitFlipVec(const VarDef &varRes,
                                  const VarDef &varRight) {
  if (!varRight.swizzle.empty())
    state.throwError(
        "NOT operator is only supported for variables!");
  VarDef zero = varRes;
  zero.reg = reg::Reg::VZERO;
  zero.type = "vec16";
  return genericLogicOp(varRes, varRight, zero, "vnor");
}

// --- Special ----------------------------------------------------------

std::vector<AsmInst> opInvertHalf(const VarDef &varRes,
                                  const VarDef &varLeft) {
  // Full vector — iterate over all 8 scalar swizzle lanes
  if (varLeft.swizzle.empty() && varRes.swizzle.empty()) {
    std::vector<AsmInst> res;
    for (char sw : {'x', 'y', 'z', 'w', 'X', 'Y', 'Z', 'W'}) {
      std::string s(1, sw);
      VarDef resCopy = varRes;
      resCopy.swizzle = s;
      VarDef leftCopy = varLeft;
      leftCopy.swizzle = s;
      auto lane = opInvertHalf(resCopy, leftCopy);
      res.insert(res.end(), lane.begin(), lane.end());
    }
    return res;
  }

  auto sitRes = SWIZZLE_MAP.find(varRes.swizzle);
  auto sitArg = SWIZZLE_MAP.find(varLeft.swizzle);
  std::string sRes =
      sitRes != SWIZZLE_MAP.end() ? sitRes->second : "";
  std::string sArg =
      sitArg != SWIZZLE_MAP.end() ? sitArg->second : "";

  if (varRes.type == "vec32" && varLeft.type == "vec16") {
    return {
        asmOp("vrcp", {fractReg(varRes) + sRes, varLeft.reg + sArg}),
        asmOp("vrcph", {intReg(varRes) + sRes, varLeft.reg + sArg})};
  }
  return {
      asmOp("vrcph", {intReg(varRes) + sRes, intReg(varLeft) + sArg}),
      asmOp("vrcpl",
            {fractReg(varRes) + sRes, fractReg(varLeft) + sArg}),
      asmOp("vrcph", {intReg(varRes) + sRes,
                      std::string(reg::Reg::VZERO) + sArg})};
}

std::vector<AsmInst> opInvertSqrtHalf(const VarDef &varRes,
                                      const VarDef &varLeft) {
  // Full vector — iterate over all 8 scalar swizzle lanes
  if (varLeft.swizzle.empty() && varRes.swizzle.empty()) {
    std::vector<AsmInst> res;
    for (char sw : {'x', 'y', 'z', 'w', 'X', 'Y', 'Z', 'W'}) {
      std::string s(1, sw);
      VarDef resCopy = varRes;
      resCopy.swizzle = s;
      VarDef leftCopy = varLeft;
      leftCopy.swizzle = s;
      auto lane = opInvertSqrtHalf(resCopy, leftCopy);
      res.insert(res.end(), lane.begin(), lane.end());
    }
    return res;
  }

  auto sitRes = SWIZZLE_MAP.find(varRes.swizzle);
  auto sitArg = SWIZZLE_MAP.find(varLeft.swizzle);
  std::string sRes =
      sitRes != SWIZZLE_MAP.end() ? sitRes->second : "";
  std::string sArg =
      sitArg != SWIZZLE_MAP.end() ? sitArg->second : "";

  return {
      asmOp("vrsqh",
            {intReg(varRes) + sRes, intReg(varLeft) + sArg}),
      asmOp("vrsql",
            {fractReg(varRes) + sRes, fractReg(varLeft) + sArg}),
      asmOp("vrsqh",
            {intReg(varRes) + sRes, std::string(reg::Reg::VZERO) +
                                        ".e0"})};
}

std::vector<AsmInst> opDivVec(const VarDef &varRes,
                              const VarDef &varLeft,
                              const VarDef &varRight) {
  state.throwError(
      "Vector division is not supported! Use invert_half() or "
      "shift-right instead.");
  return {};
}

// --- Compare ----------------------------------------------------------

std::vector<AsmInst> opCompareVec(const VarDef &varRes,
                                  const VarDef &varLeft,
                                  const VarDef &varRight,
                                  const std::string &op,
                                  const ast::TernaryPart *ternary) {
  if (!ternary && isTwoRegType(varRes.type))
    state.throwError("Vector comparison can only use vec16!");
  if (varLeft.type != "vec16")
    state.throwError("Vector comparison can only use vec16!");
  if (varRight.type != "vec16" && !varRight.reg.empty())
    state.throwError("Vector comparison can only use vec16!");
  if (!varRes.swizzle.empty())
    state.throwError(
        "Vector comparison result variable cannot use swizzle!");
  if (!varLeft.swizzle.empty())
    state.throwError(
        "Vector comparison left-side cannot use swizzle!");

  static const std::unordered_map<std::string, std::string> opMap = {
      {"<", "vlt"}, {"==", "veq"}, {"!=", "vne"}, {">=", "vge"},
  };
  auto it = opMap.find(op);
  if (it == opMap.end())
    state.throwError("Unsupported comparison operator: " + op);

  std::string swizzleRight;
  if (!varRight.swizzle.empty()) {
    auto sit = SWIZZLE_MAP.find(varRight.swizzle);
    if (sit != SWIZZLE_MAP.end()) swizzleRight = sit->second;
  }

  std::string dstReg = ternary ? reg::Reg::VTEMP0 : varRes.reg;
  std::vector<AsmInst> res;
  res.push_back(asmOp(it->second,
                {dstReg, varLeft.reg, varRight.reg + swizzleRight}));
  if (ternary) {
    // Emit select (vmrg) for ternary
    VarDef vLeft, vRight;
    if (ternary->left == "VZERO") {
      vLeft.reg = reg::Reg::VZERO;
      vLeft.type = "vec16";
    } else {
      vLeft = state.getRequiredVarCopy(ternary->left, "ternary-left");
    }
    if (ternary->rightVal.has_value()) {
      auto pIt = POW2_SWIZZLE_VAR.find(ternary->rightVal.value());
      if (pIt != POW2_SWIZZLE_VAR.end()) {
        vRight.reg = pIt->second.reg;
        vRight.swizzle = pIt->second.swizzle;
        vRight.type = "vec16";
      }
    } else if (!ternary->right.empty()) {
      vRight = state.getRequiredVarCopy(ternary->right, "ternary-right");
      vRight.swizzle = ternary->swizzleRight;
    }
    auto regsDst = getVec32Regs(varRes);
    auto regsL = getVec32Regs(vLeft);
    auto regsR = getVec32Regs(vRight);
    adjustRegsForResType(varRes, vLeft, vRight, regsL, regsR);
    auto sit = SWIZZLE_MAP.find(vRight.swizzle);
    std::string swSuffix = sit != SWIZZLE_MAP.end() ? sit->second : "";
    if (swSuffix == ".v") swSuffix = "";
    res.push_back(asmOp("vmrg", {regsDst.first, regsL.first,
                                  regsR.first + swSuffix}));
    res.push_back(asmOp("vmrg", {regsDst.second, regsL.second,
                                  regsR.second + swSuffix}));
  }
  return res;
}

} // namespace rspl::ops

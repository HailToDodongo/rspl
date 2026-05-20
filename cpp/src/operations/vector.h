#pragma once

#include "../asm.h"
#include "../ast.h"
#include "../state.h"

#include <string>
#include <utility>
#include <vector>

namespace rspl::ops {

// --- Vec32 helpers (used by builtins too) -----------------------------

std::pair<std::string, std::string> getVec32Regs(const VarDef &v);

// Move/assign (vector->vector, scalar->vector, constant->vector)
std::vector<AsmInst> opMoveVec(const VarDef &varRes,
                               const VarDef &varRight);

// Load from memory
std::vector<AsmInst> opLoadVec(const VarDef &varRes, const VarOrMem &varLoc,
                               const VarOrMem &varOffset,
                               const std::string &swizzle,
                               bool isPackedByte = false,
                               bool isSigned = true,
                               bool isUnaligned = false);
std::vector<AsmInst> opLoadBytes(const VarDef &varRes,
                                 const VarOrMem &varLoc,
                                 const VarOrMem &varOffset,
                                 const std::string &swizzle, bool isSigned);

// Store to memory
std::vector<AsmInst> opStoreVec(const VarDef &varRes,
                                const std::vector<VarOrMem> &varOffsets,
                                bool isPackedByte = false,
                                bool isSigned = true,
                                bool isUnaligned = false);
std::vector<AsmInst> opStoreBytes(const VarDef &varRes,
                                  const std::vector<VarOrMem> &varOffsets,
                                  bool isSigned);

// Arithmetic
std::vector<AsmInst> opAddVec(const VarDef &varRes, const VarDef &varLeft,
                              VarDef varRight);
std::vector<AsmInst> opSubVec(const VarDef &varRes, const VarDef &varLeft,
                              VarDef varRight);
std::vector<AsmInst> opMulVec(const VarDef &varRes, const VarDef &varLeft,
                              VarDef varRight,
                              bool clearAccum = true);

// Shifts
std::vector<AsmInst> opShiftLeftVec(const VarDef &varRes,
                                    const VarDef &varLeft,
                                    const VarDef &varRight);
std::vector<AsmInst> opShiftRightVec(const VarDef &varRes,
                                     const VarDef &varLeft,
                                     const VarDef &varRight, bool logical);

// Bitwise
std::vector<AsmInst> opAndVec(const VarDef &varRes, const VarDef &varLeft,
                              const VarDef &varRight);
std::vector<AsmInst> opOrVec(const VarDef &varRes, const VarDef &varLeft,
                             const VarDef &varRight);
std::vector<AsmInst> opNORVec(const VarDef &varRes, const VarDef &varLeft,
                              const VarDef &varRight);
std::vector<AsmInst> opXORVec(const VarDef &varRes, const VarDef &varLeft,
                              const VarDef &varRight);
std::vector<AsmInst> opBitFlipVec(const VarDef &varRes,
                                  const VarDef &varRight);

// Special
std::vector<AsmInst> opInvertHalf(const VarDef &varRes,
                                  const VarDef &varLeft);
std::vector<AsmInst> opInvertSqrtHalf(const VarDef &varRes,
                                      const VarDef &varLeft);
std::vector<AsmInst> opDivVec(const VarDef &varRes, const VarDef &varLeft,
                              const VarDef &varRight);

// Compare (vector -> stores result in varRes, optionally applies ternary)
std::vector<AsmInst> opCompareVec(const VarDef &varRes,
                                  const VarDef &varLeft,
                                  const VarDef &varRight,
                                  const std::string &op,
                                  const ast::TernaryPart *ternary);

} // namespace rspl::ops

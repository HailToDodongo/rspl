#pragma once

#include "../asm.h"
#include "../state.h"

#include <string>
#include <vector>

namespace rspl::ops {

// Load a 32-bit integer into a register with minimal instructions.
std::vector<AsmInst> loadImmediate(const std::string &regDst,
                                   const std::string &value);

// Move / assign (scalar to scalar, or immediate to scalar)
std::vector<AsmInst> opMove(const VarDef &varRes, const VarDef &varRight);

// Load from memory
std::vector<AsmInst> opLoad(const VarDef &varRes, const VarOrMem &varLoc,
                            const VarOrMem &varOffset);

// Store to memory
std::vector<AsmInst> opStore(const VarDef &varRes,
                             const std::vector<VarOrMem> &varOffsets);

// Arithmetic
std::vector<AsmInst> opAdd(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight);
std::vector<AsmInst> opSub(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight);
std::vector<AsmInst> opMul(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight,
                           bool clearAccum = true);
std::vector<AsmInst> opDiv(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight);

// Shifts
std::vector<AsmInst> opShiftLeft(const VarDef &varRes, const VarDef &varLeft,
                                 const VarDef &varRight);
std::vector<AsmInst> opShiftRight(const VarDef &varRes,
                                  const VarDef &varLeft,
                                  const VarDef &varRight, bool logical);

// Bitwise
std::vector<AsmInst> opAnd(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight);
std::vector<AsmInst> opOr(const VarDef &varRes, const VarDef &varLeft,
                          const VarDef &varRight);
std::vector<AsmInst> opNOR(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight);
std::vector<AsmInst> opXOR(const VarDef &varRes, const VarDef &varLeft,
                           const VarDef &varRight);
std::vector<AsmInst> opBitFlip(const VarDef &varRes,
                               const VarDef &varRight);

// Compare (scalar in if/while conditions)
std::vector<AsmInst> opCompare(const VarDef &varRes, const VarDef &varLeft,
                               const VarDef &varRight, const std::string &op,
                               bool ternary);

} // namespace rspl::ops

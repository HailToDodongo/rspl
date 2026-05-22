#pragma once

#include "../asm.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

// 295-bit register mask: 5 × 64-bit words
using RegMask = std::array<uint64_t, 5>;

// Fast register index lookup: register name -> index (0–294), -1 if unknown
int getRegIndex(const std::string &name);
constexpr int REG_INDEX_SIZE = 295;

// Compact stall index lookup: register name -> index (0–63), -1 if unknown
int getRegStallIndex(const std::string &name);
int getRegStallIndex(const char *name, size_t len);

// Hidden registers (read/written implicitly by certain ops)
extern const std::unordered_map<std::string, std::vector<std::string>>
    HIDDEN_REGS_READ;
extern const std::unordered_map<std::string, std::vector<std::string>>
    HIDDEN_REGS_WRITE;

// Lane expansion for vector registers
const std::vector<std::string> &expandRegister(const std::string &regName);

// Get source/target register names for an instruction
std::vector<std::string> getSourceRegs(const AsmInst &inst);
std::vector<std::string> getTargetRegs(const AsmInst &inst);

// Initialize dependency masks/indices for a single instruction
void asmInitDep(AsmInst &inst);

// Initialize dependency data for all instructions in a function
void asmInitDeps(AsmFunc &func);

// Get set of indices where instruction at position `i` can be safely reordered
std::vector<int> asmGetReorderIndices(const std::vector<AsmInst> &asmList,
                                      int i);

// Debug: scan and set min/max reorder info for each instruction
void asmScanDeps(AsmFunc &func);

} // namespace rspl

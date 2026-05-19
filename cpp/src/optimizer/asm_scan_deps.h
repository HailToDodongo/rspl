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

// Register index map: register name -> index (0–294)
extern const std::unordered_map<std::string, int> REG_INDEX_MAP;
constexpr int REG_INDEX_SIZE = 295;

// Compact stall index map: register name -> index (0–63)
extern const std::unordered_map<std::string, int> REG_STALL_INDEX_MAP;

// Hidden registers (read/written implicitly by certain ops)
extern const std::unordered_map<std::string, std::vector<std::string>>
    HIDDEN_REGS_READ;
extern const std::unordered_map<std::string, std::vector<std::string>>
    HIDDEN_REGS_WRITE;

// Lane expansion for vector registers
std::vector<std::string> expandRegister(const std::string &regName);

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

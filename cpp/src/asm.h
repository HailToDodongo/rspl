#pragma once

#include "types.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace rspl {

// --- ASM type enum ----------------------------------------------------

enum AsmType : uint8_t { OP = 0, LABEL = 1, INLINE = 3 };

// --- Op flags (bitfield) ----------------------------------------------

enum OpFlag : uint32_t {
  OP_FLAG_IS_LOAD = 1u << 0,
  OP_FLAG_IS_STORE = 1u << 1,
  OP_FLAG_IS_BRANCH = 1u << 2,
  OP_FLAG_IS_IMMOVABLE = 1u << 3,
  OP_FLAG_IS_MEM_STALL_LOAD = 1u << 4,
  OP_FLAG_IS_MEM_STALL_STORE = 1u << 5,
  OP_FLAG_IS_VECTOR = 1u << 6,
  OP_FLAG_IS_NOP = 1u << 7,
  OP_FLAG_IS_LIKELY = 1u << 8,
  OP_FLAG_LIKELY_BRANCH = 1u << 9,
  OP_FLAG_CTC2_CFC2 = 1u << 10,
};

// --- Annotation on an instruction -------------------------------------

struct AsmAnnotation {
  std::string name;
  std::string value;
};

// --- Debug information ------------------------------------------------

struct AsmDebug {
  int lineASM = 0;
  int lineRSPL = 0;
  int lineASMOpt = 0;
  int reorderCount = 0;
  int reorderLineMin = 0;
  int reorderLineMax = 0;
  int cycle = 0;
  int stall = 0;
  bool paired = false;
};

// --- Cold metadata — shared between clones via shared_ptr to avoid ---------
// deep-copying label/annotations during optimization cloning.

struct AsmInstCold {
  std::string label;
  std::string labelEnd;
  std::vector<std::string> funcArgs;
  std::vector<AsmAnnotation> annotations;
};

// --- ASM instruction --------------------------------------------------

struct AsmInst {
  std::string op;                     // mnemonic e.g. "add", "beq", "nop"
  std::vector<std::string> args;      // operands
  AsmType type = AsmType::OP;

  uint32_t opFlags = 0;               // bitfield of OpFlag
  int stallLatency = 0;

  AsmDebug debug;
  uint32_t barrierMask = 0;

  // -- Dependency tracking (filled by optimizer) -----------------------
  std::vector<int> depsSourceIdx;
  std::vector<int> depsTargetIdx;
  std::vector<int> depsStallSourceIdx;
  std::vector<int> depsStallTargetIdx;

  // 295-bit register masks stored as 5 x uint64_t
  std::array<uint64_t, 5> depsSourceMask = {};
  std::array<uint64_t, 5> depsTargetMask = {};

  uint32_t depsStallSourceMask0 = 0;
  uint32_t depsStallSourceMask1 = 0;
  uint32_t depsStallTargetMask0 = 0;
  uint32_t depsStallTargetMask1 = 0;

  // Cold metadata — shallow-copied during clone (shared_ptr refcount bump)
  std::shared_ptr<AsmInstCold> cold = std::make_shared<AsmInstCold>();
};

// --- Function-level ASM -----------------------------------------------

struct AsmFunc {
  std::string name;
  FuncType type = FuncType::Function; // function, command, or macro
  std::vector<AsmInst> asm_;
  int argSize = 0;
  int cyclesBefore = 0;
  int cyclesAfter = 0;
  std::vector<AsmAnnotation> annotations; // from AST
  std::optional<int64_t> resultType;
  std::string nameOverride; // for command aliasing

  // Temp iteration count for reorder worker
  int _iterCount = 0;
};

// --- ASM output -------------------------------------------------------

struct AsmOutput {
  std::string asm_;
  int sizeDMEM = 0;
  int sizeIMEM = 0;
};

// --- Factory functions ------------------------------------------------

AsmInst asmOp(const std::string &op,
              const std::vector<std::string> &args = {});
AsmInst asmNOP();
AsmInst asmLabel(const std::string &label);
AsmInst asmBranch(const std::string &op,
                  const std::vector<std::string> &args,
                  const std::string &labelEnd);
AsmInst asmInline(const std::string &op,
                  const std::vector<std::string> &args = {});
AsmInst asmFunction(const std::string &target,
                    const std::vector<std::string> &argRegs,
                    bool relative = false);

// --- Op classification helpers ----------------------------------------

int getStallLatency(const std::string &op);
uint32_t getOpFlags(const std::string &op);

} // namespace rspl

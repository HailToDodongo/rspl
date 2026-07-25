#pragma once

#include "types.h"

#include <array>
#include <cassert>
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

// --- Compact opcode representation ------------------------------------
// Replaces std::string op (32 bytes) with a uint16_t index (2 bytes).
// Opcode strings are interned in a global table; getOpcode() converts
// string → index, getOpcodeName() converts index → string (for output).

using Opcode = uint16_t;
Opcode getOpcode(const std::string &name);
const std::string &getOpcodeName(Opcode op);

// Cached Opcode constants for common comparisons (fast after first use).
// Use: inst.op == OP_NOP instead of inst.op == getOpcode("nop").
namespace Op {
  inline Opcode NOP()   { static Opcode o = getOpcode("nop");    return o; }
  inline Opcode J()     { static Opcode o = getOpcode("j");      return o; }
  inline Opcode JR()    { static Opcode o = getOpcode("jr");     return o; }
  inline Opcode JAL()   { static Opcode o = getOpcode("jal");    return o; }
  inline Opcode BEQ()   { static Opcode o = getOpcode("beq");    return o; }
  inline Opcode BNE()   { static Opcode o = getOpcode("bne");    return o; }
  inline Opcode MTC2()  { static Opcode o = getOpcode("mtc2");   return o; }
  inline Opcode MTC0()  { static Opcode o = getOpcode("mtc0");   return o; }
  inline Opcode CTC2()  { static Opcode o = getOpcode("ctc2");   return o; }
  inline Opcode STV()   { static Opcode o = getOpcode("stv");    return o; }
  inline Opcode LTV()   { static Opcode o = getOpcode("ltv");    return o; }
  inline Opcode LUI()   { static Opcode o = getOpcode("lui");    return o; }
  inline Opcode ADDIU() { static Opcode o = getOpcode("addiu");  return o; }
  inline Opcode ADDU()  { static Opcode o = getOpcode("addu");   return o; }
  inline Opcode ORI()   { static Opcode o = getOpcode("ori");    return o; }
  inline Opcode VMUDL() { static Opcode o = getOpcode("vmudl");  return o; }
  inline Opcode VXOR()  { static Opcode o = getOpcode("vxor");   return o; }
  inline Opcode VRSQH() { static Opcode o = getOpcode("vrsqh");  return o; }
  inline Opcode VRCPH() { static Opcode o = getOpcode("vrcph");  return o; }
  inline Opcode VRSQL() { static Opcode o = getOpcode("vrsql");  return o; }
  inline Opcode VRCPL() { static Opcode o = getOpcode("vrcpl");  return o; }
}

// --- SmallVec: fixed-capacity vector, no heap allocation ---------

template <typename T, size_t N> struct SmallVec {
  std::array<T, N> data_{};
  uint8_t size_ = 0;
  T &operator[](size_t i) { return data_[i]; }
  const T &operator[](size_t i) const { return data_[i]; }
  T *begin() { return data_.data(); }
  T *end() { return data_.data() + size_; }
  const T *begin() const { return data_.data(); }
  const T *end() const { return data_.data() + size_; }
  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  void push_back(const T &v) { assert(size_ < N); data_[size_++] = v; }
  void clear() { size_ = 0; }
  T &back() { return data_[size_ - 1]; }
  const T &back() const { return data_[size_ - 1]; }
  void pop_back() { --size_; }
  SmallVec &operator=(const std::vector<T> &v) {
    size_ = 0;
    for (const auto &e : v) push_back(e);
    return *this;
  }
};

// --- ASM instruction --------------------------------------------------

struct AsmInst {
  Opcode op = 0;                      // mnemonic e.g. "add", "beq", "nop"
  std::vector<std::string> args;      // operands
  AsmType type = AsmType::OP;

  uint32_t opFlags = 0;               // bitfield of OpFlag
  int stallLatency = 0;

  AsmDebug debug;
  uint32_t barrierMask = 0;

  // -- Dependency tracking (filled by optimizer) -----------------------
  // Capacity must cover the worst case after lane-expansion: two vector args
  // at 8 lanes each plus two hidden regs (e.g. "vcl" reading $vco and $vce).
  SmallVec<int, 24> depsSourceIdx;
  SmallVec<int, 24> depsTargetIdx;
  SmallVec<int, 24> depsStallSourceIdx;
  SmallVec<int, 24> depsStallTargetIdx;

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

int getStallLatency(Opcode op);
uint32_t getOpFlags(Opcode op);

} // namespace rspl

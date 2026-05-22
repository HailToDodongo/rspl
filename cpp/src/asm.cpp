#include "asm.h"
#include "state.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace rspl {

// --- Opcode registry ---------------------------------------------------
// Function-local statics to avoid static-init-order fiasco across TUs.

struct OpcodeRegistry {
  std::vector<std::string> names;
  std::unordered_map<std::string, Opcode> map;
};
static OpcodeRegistry &opcodeReg() {
  static OpcodeRegistry reg;
  return reg;
}

Opcode getOpcode(const std::string &name) {
  if (name.empty()) return 0;
  auto &reg = opcodeReg();
  auto it = reg.map.find(name);
  if (it != reg.map.end()) return it->second;
  Opcode idx = static_cast<Opcode>(reg.names.size() + 1);
  reg.names.push_back(name);
  reg.map[name] = idx;
  return idx;
}

const std::string &getOpcodeName(Opcode op) {
  static const std::string empty;
  auto &names = opcodeReg().names;
  if (op == 0 || op > names.size()) return empty;
  return names[op - 1];
}

// --- Precomputed opcode → (flags, latency) map --------------------------
// Replaces the 8 separate unordered_set lookups with a single map lookup.

struct OpInfoEntry { uint32_t flags; int latency; };

static const std::unordered_map<Opcode, OpInfoEntry> OP_INFO_MAP = []() {
  std::unordered_map<Opcode, OpInfoEntry> m;

  auto add = [&](const char *op, uint32_t flags, int latency) {
    m[getOpcode(op)] = {flags, latency};
  };

  // Branches
  for (auto *op : {"beq","bne","bgezal","bltzal","bgez","bltz","blez","bgtz",
                   "j","jr","jal"})
    add(op, OP_FLAG_IS_BRANCH | OP_FLAG_IS_IMMOVABLE, 0);

  // Stores (also get MEM_STALL_STORE)
  for (auto *op : {"sw","sh","sb","sbv","ssv","slv","sdv","sqv","spv","suv",
                   "shv","sfv","stv","swv","srv"})
    add(op, OP_FLAG_IS_STORE | OP_FLAG_IS_MEM_STALL_STORE, 0);

  // Vector loads
  for (auto *op : {"lbv","lsv","llv","ldv","lqv","lpv","luv","lhv","lfv",
                   "ltv","lrv"})
    add(op, OP_FLAG_IS_LOAD | OP_FLAG_IS_MEM_STALL_LOAD, 4);

  // Scalar loads
  for (auto *op : {"lw","lh","lhu","lb","lbu"})
    add(op, OP_FLAG_IS_LOAD | OP_FLAG_IS_MEM_STALL_LOAD, 3);

  // Stall ops (both load and store stalls)
  for (auto *op : {"mfc0","mtc0","mfc2","mtc2","cfc2","ctc2"}) {
    uint32_t f = OP_FLAG_IS_MEM_STALL_LOAD | OP_FLAG_IS_MEM_STALL_STORE;
    int lat = 3;
    if (op[2] == 'c') { // cfc2/ctc2
      f |= OP_FLAG_CTC2_CFC2;
    }
    if (op[1] == 't' && op[2] == 'c' && op[3] == '2') lat = 4; // mtc2
    add(op, f, lat);
  }

  // Special
  add("nop", OP_FLAG_IS_NOP | OP_FLAG_IS_IMMOVABLE, 0);
  add("catch", OP_FLAG_IS_MEM_STALL_LOAD | OP_FLAG_IS_MEM_STALL_STORE, 0);

  return m;
}();

int getStallLatency(Opcode op) {
  auto it = OP_INFO_MAP.find(op);
  if (it != OP_INFO_MAP.end()) return it->second.latency;
  const auto &name = getOpcodeName(op);
  if (!name.empty() && name[0] == 'v') return 4;
  return 0;
}

uint32_t getOpFlags(Opcode op) {
  uint32_t flags = OP_FLAG_IS_LIKELY;
  auto it = OP_INFO_MAP.find(op);
  if (it != OP_INFO_MAP.end()) flags |= it->second.flags;
  else {
    const auto &name = getOpcodeName(op);
    if (!name.empty() && name[0] == 'v') flags |= OP_FLAG_IS_VECTOR;
  }
  if (op == getOpcode("nop")) flags |= OP_FLAG_IS_NOP;

  if ((flags & OP_FLAG_IS_BRANCH) && (flags & OP_FLAG_IS_LIKELY))
    flags |= OP_FLAG_LIKELY_BRANCH;
  return flags;
}

static AsmDebug currentDebug() {
  return AsmDebug{.lineRSPL = static_cast<int>(state.line)};
}

static void applyOpInfo(AsmInst &inst, Opcode op,
                        AsmType type) {
  inst.type = type;
  inst.opFlags = getOpFlags(op);
  inst.stallLatency = getStallLatency(op);
  // Copy current annotations from state (but don't clear —
  // clearing is done per-statement in scopedBlockToAsm to match JS)
  for (const auto &ann : state.getAnnotations()) {
    inst.cold->annotations.push_back({ann.name, ann.value});
  }
}

// --- Factory functions ------------------------------------------------

AsmInst asmOp(const std::string &op,
              const std::vector<std::string> &args) {
  AsmInst inst;
  inst.op = getOpcode(op);
  inst.args = args;
  inst.debug = currentDebug();
  applyOpInfo(inst, inst.op, AsmType::OP);
  return inst;
}

AsmInst asmNOP() {
  AsmInst inst;
  inst.op = getOpcode("nop");
  inst.debug = currentDebug();
  applyOpInfo(inst, inst.op, AsmType::OP);
  return inst;
}

AsmInst asmLabel(const std::string &label) {
  AsmInst inst;
  inst.cold->label = label;
  inst.op = 0;
  inst.debug = currentDebug();
  applyOpInfo(inst, 0, AsmType::LABEL);
  return inst;
}

AsmInst asmBranch(const std::string &op,
                  const std::vector<std::string> &args,
                  const std::string &labelEnd) {
  AsmInst inst = asmOp(op, args);
  inst.cold->labelEnd = labelEnd;
  return inst;
}

AsmInst asmInline(const std::string &op,
                  const std::vector<std::string> &args) {
  AsmInst inst;
  inst.op = getOpcode(op);
  inst.args = args;
  inst.debug = currentDebug();
  applyOpInfo(inst, inst.op, AsmType::INLINE);
  return inst;
}

AsmInst asmFunction(const std::string &target,
                    const std::vector<std::string> &argRegs,
                    bool relative) {
  if (relative) {
    AsmInst inst = asmOp("bgezal", {"$zero", target});
    inst.cold->funcArgs = argRegs;
    return inst;
  }
  AsmInst inst = asmOp("jal", {target});
  inst.cold->funcArgs = argRegs;
  return inst;
}

} // namespace rspl

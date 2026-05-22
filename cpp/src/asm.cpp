#include "asm.h"
#include "state.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace rspl {

// --- Precomputed opcode → (flags, latency) map --------------------------
// Replaces the 8 separate unordered_set lookups with a single map lookup.

struct OpInfoEntry { uint32_t flags; int latency; };

static const std::unordered_map<std::string, OpInfoEntry> OP_INFO_MAP = []() {
  std::unordered_map<std::string, OpInfoEntry> m;

  // For the few ops that need to be in the map even with flags=0
  auto add = [&](const char *op, uint32_t flags, int latency) {
    m[op] = {flags, latency};
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

int getStallLatency(const std::string &op) {
  if (!op.empty() && op[0] == 'v') return 4;
  auto it = OP_INFO_MAP.find(op);
  return it != OP_INFO_MAP.end() ? it->second.latency : 0;
}

uint32_t getOpFlags(const std::string &op) {
  uint32_t flags = OP_FLAG_IS_LIKELY;
  if (!op.empty() && op[0] == 'v') flags |= OP_FLAG_IS_VECTOR;
  if (op == "nop") flags |= OP_FLAG_IS_NOP;

  auto it = OP_INFO_MAP.find(op);
  if (it != OP_INFO_MAP.end()) flags |= it->second.flags;

  if ((flags & OP_FLAG_IS_BRANCH) && (flags & OP_FLAG_IS_LIKELY))
    flags |= OP_FLAG_LIKELY_BRANCH;
  return flags;
}

static AsmDebug currentDebug() {
  return AsmDebug{.lineRSPL = static_cast<int>(state.line)};
}

static void applyOpInfo(AsmInst &inst, const std::string &op,
                        AsmType type) {
  inst.type = type;
  inst.opFlags = getOpFlags(op);
  inst.stallLatency = getStallLatency(op);
  // Copy current annotations from state (but don't clear —
  // clearing is done per-statement in scopedBlockToAsm to match JS)
  for (const auto &ann : state.getAnnotations()) {
    inst.annotations.push_back({ann.name, ann.value});
  }
}

// --- Factory functions ------------------------------------------------

AsmInst asmOp(const std::string &op,
              const std::vector<std::string> &args) {
  AsmInst inst;
  inst.op = op;
  inst.args = args;
  inst.debug = currentDebug();
  applyOpInfo(inst, op, AsmType::OP);
  return inst;
}

AsmInst asmNOP() {
  AsmInst inst;
  inst.op = "nop";
  inst.debug = currentDebug();
  applyOpInfo(inst, "nop", AsmType::OP);
  return inst;
}

AsmInst asmLabel(const std::string &label) {
  AsmInst inst;
  inst.label = label;
  inst.op = "";
  inst.debug = currentDebug();
  applyOpInfo(inst, "", AsmType::LABEL);
  return inst;
}

AsmInst asmBranch(const std::string &op,
                  const std::vector<std::string> &args,
                  const std::string &labelEnd) {
  AsmInst inst = asmOp(op, args);
  inst.labelEnd = labelEnd;
  return inst;
}

AsmInst asmInline(const std::string &op,
                  const std::vector<std::string> &args) {
  AsmInst inst;
  inst.op = op;
  inst.args = args;
  inst.debug = currentDebug();
  applyOpInfo(inst, op, AsmType::INLINE);
  return inst;
}

AsmInst asmFunction(const std::string &target,
                    const std::vector<std::string> &argRegs,
                    bool relative) {
  if (relative) {
    AsmInst inst = asmOp("bgezal", {"$zero", target});
    inst.funcArgs = argRegs;
    return inst;
  }
  AsmInst inst = asmOp("jal", {target});
  inst.funcArgs = argRegs;
  return inst;
}

} // namespace rspl

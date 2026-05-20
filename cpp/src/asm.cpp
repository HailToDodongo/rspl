#include "asm.h"
#include "state.h"

#include <algorithm>
#include <unordered_set>

namespace rspl {

// --- Op classification sets -------------------------------------------

static const std::unordered_set<std::string> STORE_OPS = {
    "sw", "sh",  "sb",  "sbv", "ssv", "slv", "sdv",
    "sqv", "spv", "suv", "shv", "sfv", "stv", "swv", "srv",
};

static const std::unordered_set<std::string> LOAD_OPS_SCALAR = {
    "lw", "lh", "lhu", "lb", "lbu",
};

static const std::unordered_set<std::string> LOAD_OPS_VECTOR = {
    "lbv", "lsv", "llv", "ldv", "lqv",
    "lpv", "luv", "lhv", "lfv", "ltv", "lrv",
};

static const std::unordered_set<std::string> BRANCH_OPS = {
    "beq", "bne", "bgezal", "bltzal", "bgez",
    "bltz", "blez", "bgtz", "j", "jr", "jal",
};

static const std::unordered_set<std::string> IMMOVABLE_OPS = {
    "beq", "bne", "bgezal", "bltzal", "bgez",
    "bltz", "blez", "bgtz", "j", "jr", "jal", "nop",
};

static const std::unordered_set<std::string> LOAD_OPS = []() {
  std::unordered_set<std::string> s;
  s.insert(LOAD_OPS_SCALAR.begin(), LOAD_OPS_SCALAR.end());
  s.insert(LOAD_OPS_VECTOR.begin(), LOAD_OPS_VECTOR.end());
  return s;
}();

static const std::unordered_set<std::string> MEM_STALL_LOAD_OPS = []() {
  auto s = LOAD_OPS;
  s.insert({"mfc0", "mtc0", "mfc2", "mtc2", "cfc2", "ctc2", "catch"});
  return s;
}();

static const std::unordered_set<std::string> MEM_STALL_STORE_OPS = []() {
  auto s = STORE_OPS;
  s.insert({"mfc0", "mtc0", "mfc2", "mtc2", "cfc2", "ctc2", "catch"});
  return s;
}();

// --- Helpers ----------------------------------------------------------

int getStallLatency(const std::string &op) {
  if (!op.empty() && op[0] == 'v') return 4;
  if (op == "mtc2") return 4;
  if (LOAD_OPS_VECTOR.count(op)) return 4;
  if (LOAD_OPS_SCALAR.count(op)) return 3;
  if (op == "mfc0" || op == "mfc2" || op == "cfc2" || op == "ctc2")
    return 3;
  return 0;
}

uint32_t getOpFlags(const std::string &op) {
  uint32_t flags = 0;
  if (LOAD_OPS.count(op)) flags |= OP_FLAG_IS_LOAD;
  if (STORE_OPS.count(op)) flags |= OP_FLAG_IS_STORE;
  if (BRANCH_OPS.count(op)) flags |= OP_FLAG_IS_BRANCH;
  if (IMMOVABLE_OPS.count(op)) flags |= OP_FLAG_IS_IMMOVABLE;
  if (MEM_STALL_LOAD_OPS.count(op)) flags |= OP_FLAG_IS_MEM_STALL_LOAD;
  if (MEM_STALL_STORE_OPS.count(op)) flags |= OP_FLAG_IS_MEM_STALL_STORE;
  if (!op.empty() && op[0] == 'v') flags |= OP_FLAG_IS_VECTOR;
  if (op == "nop") flags |= OP_FLAG_IS_NOP;
  flags |= OP_FLAG_IS_LIKELY; // default true unless @Unlikely annotation
  if (op == "cfc2" || op == "ctc2") flags |= OP_FLAG_CTC2_CFC2;
  if ((flags & OP_FLAG_IS_BRANCH) && (flags & OP_FLAG_IS_LIKELY)) {
    flags |= OP_FLAG_LIKELY_BRANCH;
  }
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

#include "asm_scan_deps.h"
#include "../registers.h"
#include "../state.h"
#include "../swizzle.h"
#include "../types.h"

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>

namespace rspl {

// --- Register index map (295 entries) ---------------------------------

const std::unordered_map<std::string, int> REG_INDEX_MAP = []() {
  std::unordered_map<std::string, int> m;
  // Vector registers with lanes: $vXX -> start index, $vXX_N -> start+N
  const char *vNames[] = {
      "$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07",
      "$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15",
      "$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23",
      "$v24", "$v25", "$v26", "$v27", "$v28", "$v29", "$v30", "$v31",
  };
  for (int i = 0; i < 32; ++i) {
    int base = i * 8;
    m[vNames[i]] = base;
    for (int lane = 0; lane < 8; ++lane) {
      m[std::string(vNames[i]) + "_" + std::to_string(lane)] =
          base + lane;
    }
  }

  // Scalar registers: $zero–$ra -> 256–287
  const char *sNames[] = {
      "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
      "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
      "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
      "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
  };
  for (int i = 0; i < 32; ++i) m[sNames[i]] = 256 + i;

  // Special registers: 288–294
  m["$vco"] = 288;
  m["$vcc"] = 289;
  m["$acc"] = 290;
  m["$divOut"] = 291;
  m["$divIn"] = 292;
  m["$divDP"] = 293;
  m["$vce"] = 294;

  return m;
}();

// --- Stall index map (64 entries) -------------------------------------

const std::unordered_map<std::string, int> REG_STALL_INDEX_MAP = []() {
  std::unordered_map<std::string, int> m;
  const char *scalars[] = {
      "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
      "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
      "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
      "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
  };
  for (int i = 0; i < 32; ++i) m[scalars[i]] = i;
  const char *vecs[] = {
      "$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07",
      "$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15",
      "$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23",
      "$v24", "$v25", "$v26", "$v27", "$v28", "$v29", "$v30", "$v31",
  };
  for (int i = 0; i < 32; ++i) m[vecs[i]] = 32 + i;
  return m;
}();

// --- Hidden registers -------------------------------------------------

const std::unordered_map<std::string, std::vector<std::string>>
    HIDDEN_REGS_READ = {
        {"vlt", {"$vco"}},
        {"veq", {"$vco"}},
        {"vne", {"$vco"}},
        {"vge", {"$vco"}},
        {"vmrg", {"$vcc"}},
        {"vcl", {"$vco", "$vce"}},
        {"vmacf", {"$acc"}},
        {"vmacu", {"$acc"}},
        {"vmadn", {"$acc"}},
        {"vmadl", {"$acc"}},
        {"vmadm", {"$acc"}},
        {"vmadh", {"$acc"}},
        {"vrndp", {"$acc"}},
        {"vrndn", {"$acc"}},
        {"vmacq", {"$acc"}},
        {"vsar", {"$acc"}},
        {"vrcph", {"$divOut"}},
        {"vrsqh", {"$divOut"}},
        {"vrcpl", {"$divIn", "$divDP"}},
        {"vrsql", {"$divIn", "$divDP"}},
        {"vadd", {"$vco"}},
        {"vsub", {"$vco"}},
};

const std::unordered_map<std::string, std::vector<std::string>>
    HIDDEN_REGS_WRITE = {
        {"vlt",  {"$vcc", "$vco", "$acc"}},
        {"veq",  {"$vcc", "$vco", "$acc"}},
        {"vne",  {"$vcc", "$vco", "$acc"}},
        {"vge",  {"$vcc", "$vco", "$acc"}},
        {"vch",  {"$vcc", "$vco", "$acc", "$vce"}},
        {"vcr",  {"$vcc", "$vco", "$acc", "$vce"}},
        {"vcl",  {"$vcc", "$vco", "$acc", "$vce"}},
        {"vmrg", {"$vco", "$acc"}},
        {"vmov", {"$acc"}},
        {"vrcp", {"$acc", "$divOut", "$divDP"}},
        {"vrcph", {"$acc", "$divIn", "$divDP"}},
        {"vrcpl", {"$acc", "$divOut", "$divDP"}},
        {"vrsq", {"$acc", "$divOut", "$divDP"}},
        {"vrsqh", {"$acc", "$divIn", "$divDP"}},
        {"vrsql", {"$acc", "$divOut", "$divDP"}},
        {"vadd",  {"$vco", "$acc"}},
        {"vsub",  {"$vco", "$acc"}},
        {"vaddc", {"$vco", "$acc"}},
        {"vsubc", {"$vco", "$acc"}},
        {"vabs",  {"$acc"}},
        {"vand",  {"$acc"}},
        {"vnand", {"$acc"}},
        {"vor",   {"$acc"}},
        {"vnor",  {"$acc"}},
        {"vxor",  {"$acc"}},
        {"vnxor", {"$acc"}},
        {"vmulf", {"$acc"}},
        {"vmulu", {"$acc"}},
        {"vmacf", {"$acc"}},
        {"vmacu", {"$acc"}},
        {"vmudn", {"$acc"}},
        {"vmadn", {"$acc"}},
        {"vmudl", {"$acc"}},
        {"vmadl", {"$acc"}},
        {"vmudm", {"$acc"}},
        {"vmadm", {"$acc"}},
        {"vmudh", {"$acc"}},
        {"vmadh", {"$acc"}},
        {"vrndp", {"$acc"}},
        {"vrndn", {"$acc"}},
        {"vmulq", {"$acc"}},
        {"vmacq", {"$acc"}},
};

static const std::unordered_set<std::string> STALL_IGNORE_REGS = {
    "$vcc", "$vco", "$acc", "$vce", "$divOut", "$divIn", "$divDP"};

static const std::unordered_set<std::string> READ_ONLY_OPS = {
    "beq",  "bne",  "bgezal", "bltzal", "bgez", "bltz",
    "blez", "bgtz", "j",      "jr",     "jal",
    "sw",   "sh",   "sb",     "sbv",    "ssv",  "slv", "sdv",
    "sqv",  "spv",  "suv",    "shv",    "sfv",  "stv", "swv", "srv",
    "mtc0",
};

// --- LTV/STV register groups ------------------------------------------

static const std::unordered_map<std::string, std::vector<std::string>>
    LTV_REG_MAP = {
        {"$v00",
         {"$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07"}},
        {"$v08",
         {"$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15"}},
        {"$v16",
         {"$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23"}},
        {"$v24",
         {"$v24", "$v25", "$v26", "$v27", "$v28", "$v29", "$v30", "$v31"}},
};

// --- Mask utilities ---------------------------------------------------

static RegMask makeMask(const std::vector<std::string> &regs) {
  RegMask m = {};
  for (const auto &r : regs) {
    auto it = REG_INDEX_MAP.find(r);
    if (it == REG_INDEX_MAP.end()) {
      state.throwError("Unknown register: " + r);
    }
    int idx = it->second;
    m[idx / 64] |= (1ULL << (idx % 64));
  }
  return m;
}

static bool maskIsZero(const RegMask &m) {
  return m[0] == 0 && m[1] == 0 && m[2] == 0 && m[3] == 0 && m[4] == 0;
}

static bool maskAnd(const RegMask &a, const RegMask &b) {
  for (int i = 0; i < 5; ++i)
    if (a[i] & b[i]) return true;
  return false;
}

static void maskOr(RegMask &dst, const RegMask &src) {
  for (int i = 0; i < 5; ++i) dst[i] |= src[i];
}

static RegMask maskAll() {
  // All bits set for all 295 registers
  RegMask m = {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
               0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
               0x7FFFFFFFULL}; // bits 0–294
  return m;
}

// --- Expand vector registers to lanes ---------------------------------

std::vector<std::string> expandRegister(const std::string &regName) {
  auto dotPos = regName.find('.');
  std::string reg = (dotPos != std::string::npos)
                        ? regName.substr(0, dotPos)
                        : regName;
  if (reg::isVecReg(reg)) {
    std::string lane =
        (dotPos != std::string::npos) ? regName.substr(dotPos + 1) : "v";
    // Look up lane name directly (with dot if present in original)
    static const std::unordered_map<std::string, std::vector<int>>
        laneMap = {
            {"v",  {0, 1, 2, 3, 4, 5, 6, 7}},
            {".q0", {0, 2, 4, 6}},        {".q1", {1, 3, 5, 7}},
            {".h0", {0, 4}},              {".h1", {1, 5}},
            {".h2", {2, 6}},              {".h3", {3, 7}},
            {".e0", {0}},                 {".e1", {1}},
            {".e2", {2}},                 {".e3", {3}},
            {".e4", {4}},                 {".e5", {5}},
            {".e6", {6}},                 {".e7", {7}},
    };
    // The lane name from the register already includes the dot for
    // .h0/.e1/etc patterns (since split on first '.').
    // For "v" (no dot in original), look it up directly.
    std::string lookupKey = (dotPos != std::string::npos && lane != "v")
                                ? "." + lane
                                : lane;
    auto it = laneMap.find(lookupKey);
    if (it == laneMap.end()) {
      // Try SWIZZLE_MAP lookup for swizzle patterns
      auto sit = SWIZZLE_MAP.find(lane);
      if (sit != SWIZZLE_MAP.end()) {
        it = laneMap.find(sit->second);
      }
    }
    if (it == laneMap.end()) return {regName};
    std::vector<std::string> result;
    for (int l : it->second) {
      result.push_back(reg + "_" + std::to_string(l));
    }
    return result;
  }
  return {regName};
}

// --- Source/target register extraction --------------------------------

static std::string extractRegFromArg(const std::string &arg) {
  auto brIdx = arg.rfind('(');
  if (brIdx != std::string::npos) {
    return arg.substr(brIdx + 1, arg.size() - brIdx - 2);
  }
  if (!arg.empty() && arg[0] == '$') return arg;
  return "";
}

std::vector<std::string> getSourceRegs(const AsmInst &inst) {
  if (inst.op == "jr" || inst.op == "mtc2" || inst.op == "mtc0" ||
      inst.op == "ctc2") {
    return {inst.args[0]};
  }
  if ((inst.opFlags & OpFlag::OP_FLAG_IS_BRANCH) &&
      inst.op.starts_with("b")) {
    if (inst.args.empty()) return {};
    return std::vector<std::string>(inst.args.begin(),
                                    inst.args.end() - 1);
  }
  if (inst.opFlags & OpFlag::OP_FLAG_IS_STORE) {
    if (inst.op == "stv") {
      const std::string &mainReg =
          inst.args.empty() ? "$v00" : inst.args[0];
      auto it = LTV_REG_MAP.find(mainReg);
      if (it == LTV_REG_MAP.end()) {
        state.throwError(
            "Invalid base register " + mainReg + " for stv!");
      }
      int row = inst.args.size() > 1 ? std::stoi(inst.args[1]) / 2 : 0;
      std::vector<std::string> regs;
      for (int i = 0; i < 8; ++i) {
        regs.push_back(it->second[i] + ".e" +
                       std::to_string((8 + i - row) % 8));
      }
      regs.push_back(inst.args.back());
      return regs;
    }
    return inst.args;
  }
  if (inst.op == "j" || inst.op == "jal") {
    return {inst.args[0]};
  }
  std::vector<std::string> res(inst.args.begin() + 1, inst.args.end());
  auto hit = HIDDEN_REGS_READ.find(inst.op);
  if (hit != HIDDEN_REGS_READ.end()) {
    res.insert(res.end(), hit->second.begin(), hit->second.end());
  }
  return res;
}

std::vector<std::string> getTargetRegs(const AsmInst &inst) {
  if (READ_ONLY_OPS.count(inst.op)) return {};

  if ((inst.opFlags & OpFlag::OP_FLAG_IS_LOAD) &&
      inst.op == "ltv") {
    const std::string &mainReg =
        inst.args.empty() ? "$v00" : inst.args[0];
    auto it = LTV_REG_MAP.find(mainReg);
    if (it == LTV_REG_MAP.end()) {
      state.throwError("Invalid base register " + mainReg + " for ltv!");
    }
    int row = inst.args.size() > 1 ? std::stoi(inst.args[1]) / 2 : 0;
    std::vector<std::string> regs;
    for (int i = 0; i < 8; ++i) {
      regs.push_back(it->second[i] + ".e" +
                     std::to_string((8 + i - row) % 8));
    }
    return regs;
  }

  const std::string &targetReg =
      (inst.op == "mtc2" || inst.op == "ctc2") ? inst.args[1]
                                                : inst.args[0];
  std::vector<std::string> res = {targetReg};
  auto hit = HIDDEN_REGS_WRITE.find(inst.op);
  if (hit != HIDDEN_REGS_WRITE.end()) {
    res.insert(res.end(), hit->second.begin(), hit->second.end());
  }
  return res;
}

// --- Dependency initialization ----------------------------------------

void asmInitDep(AsmInst &inst) {
  if (inst.type != AsmType::OP || (inst.opFlags & OpFlag::OP_FLAG_IS_NOP)) {
    inst.depsSourceIdx.clear();
    inst.depsTargetIdx.clear();
    inst.depsStallSourceIdx.clear();
    inst.depsStallTargetIdx.clear();
    std::fill(std::begin(inst.depsSourceMask),
              std::end(inst.depsSourceMask), 0);
    std::fill(std::begin(inst.depsTargetMask),
              std::end(inst.depsTargetMask), 0);
    std::fill(std::begin(inst.depsBlockSourceMask),
              std::end(inst.depsBlockSourceMask), 0);
    std::fill(std::begin(inst.depsBlockTargetMask),
              std::end(inst.depsBlockTargetMask), 0);
    inst.depsStallSourceMask0 = 0;
    inst.depsStallSourceMask1 = 0;
    inst.depsStallTargetMask0 = 0;
    inst.depsStallTargetMask1 = 0;
    inst.barrierMask = 0;
    return;
  }

  // In JS: depsStallSource = getSourceRegs (what we READ)
  //        depsStallTarget = getTargetRegs (what we WRITE)
  auto rawSrc = getSourceRegs(inst);
  // Filter to only $regs
  std::vector<std::string> depsStallSource;
  for (auto &r : rawSrc) {
    std::string ext = extractRegFromArg(r);
    if (!ext.empty() && ext[0] == '$') depsStallSource.push_back(ext);
  }
  // Deduplicate
  std::unordered_set<std::string> seenSrc(depsStallSource.begin(),
                                          depsStallSource.end());
  depsStallSource.assign(seenSrc.begin(), seenSrc.end());

  auto rawTgt = getTargetRegs(inst);
  std::vector<std::string> depsStallTarget(rawTgt.begin(), rawTgt.end());

  // Expand to lanes
  std::vector<std::string> depsSource;
  for (auto &r : depsStallSource) {
    auto expanded = expandRegister(r);
    depsSource.insert(depsSource.end(), expanded.begin(),
                      expanded.end());
  }
  std::vector<std::string> depsTarget;
  for (auto &r : depsStallTarget) {
    auto expanded = expandRegister(r);
    depsTarget.insert(depsTarget.end(), expanded.begin(),
                      expanded.end());
  }

  inst.depsSourceMask = makeMask(depsSource);
  inst.depsTargetMask = makeMask(depsTarget);

  // Build indices
  for (auto &r : depsSource) {
    auto it = REG_INDEX_MAP.find(r);
    if (it != REG_INDEX_MAP.end()) inst.depsSourceIdx.push_back(it->second);
  }
  for (auto &r : depsTarget) {
    auto it = REG_INDEX_MAP.find(r);
    if (it != REG_INDEX_MAP.end()) inst.depsTargetIdx.push_back(it->second);
  }

  // Stall-wise: strip lane and filter special regs
  std::vector<std::string> stallSrc, stallTgt;
  for (auto &r : depsStallSource) {
    auto dotPos = r.find('.');
    std::string base = (dotPos != std::string::npos)
                           ? r.substr(0, dotPos)
                           : r;
    if (!STALL_IGNORE_REGS.count(base)) stallSrc.push_back(base);
  }
  for (auto &r : depsStallTarget) {
    auto dotPos = r.find('.');
    std::string base = (dotPos != std::string::npos)
                           ? r.substr(0, dotPos)
                           : r;
    if (!STALL_IGNORE_REGS.count(base)) stallTgt.push_back(base);
  }

  for (auto &r : stallSrc) {
    auto it = REG_STALL_INDEX_MAP.find(r);
    if (it != REG_STALL_INDEX_MAP.end())
      inst.depsStallSourceIdx.push_back(it->second);
  }
  for (auto &r : stallTgt) {
    auto it = REG_STALL_INDEX_MAP.find(r);
    if (it != REG_STALL_INDEX_MAP.end())
      inst.depsStallTargetIdx.push_back(it->second);
  }

  // Stall masks as 2×uint32
  uint64_t srcMask = 0, tgtMask = 0;
  for (int idx : inst.depsStallSourceIdx)
    srcMask |= (1ULL << idx);
  for (int idx : inst.depsStallTargetIdx)
    tgtMask |= (1ULL << idx);
  inst.depsStallSourceMask0 = static_cast<uint32_t>(srcMask);
  inst.depsStallSourceMask1 = static_cast<uint32_t>(srcMask >> 32);
  inst.depsStallTargetMask0 = static_cast<uint32_t>(tgtMask);
  inst.depsStallTargetMask1 = static_cast<uint32_t>(tgtMask >> 32);

  // Barrier mask from annotations
  inst.barrierMask = 0;
  for (auto &ann : inst.annotations) {
    if (ann.name == "Barrier") {
      inst.barrierMask |= state.getBarrierMask(ann.value);
    }
  }
}

void asmInitDeps(AsmFunc &func) {
  for (auto &inst : func.asm_) asmInitDep(inst);
  // Block init skipped for now
}

// --- Reorder indices --------------------------------------------------

static bool checkAsmBackwardDep(const AsmInst &asm_,
                                const AsmInst &asmPrev) {
  if (asm_.type != AsmType::OP || asmPrev.type != AsmType::OP)
    return true;
  if (maskAnd(asmPrev.depsTargetMask, asm_.depsSourceMask)) return true;
  if (maskAnd(asmPrev.depsSourceMask, asm_.depsTargetMask)) return true;
  if (asm_.barrierMask & asmPrev.barrierMask) return true;
  return false;
}

std::vector<int> asmGetReorderIndices(const std::vector<AsmInst> &asmList,
                                      int i) {
  const AsmInst &asm_ = asmList[i];
  if (asm_.opFlags & OpFlag::OP_FLAG_IS_IMMOVABLE) return {i};

  std::vector<int> res;
  int pos = asmList.size();

  // Scan forward
  for (int f = i + 1; f < (int)asmList.size(); ++f) {
    const AsmInst &asmNext = asmList[f];
    const AsmInst *prevPrev =
        (f >= 2) ? &asmList[f - 2] : nullptr;

    bool isFilledBranch =
        (asmNext.opFlags & OpFlag::OP_FLAG_IS_BRANCH) &&
        !((f + 1 < (int)asmList.size()) &&
          (asmList[f + 1].opFlags & OpFlag::OP_FLAG_IS_NOP));
    bool isPastBranch =
        prevPrev && (prevPrev->opFlags & OpFlag::OP_FLAG_IS_BRANCH);

    if (isFilledBranch || isPastBranch ||
        checkAsmBackwardDep(asmNext, asm_)) {
      pos = f;
      break;
    }
  }

  for (int r = i; r <= pos - 1; ++r) res.push_back(r);

  // Scan backward
  for (int b = i - 1; b >= 0; --b) {
    const AsmInst &asmPrev = asmList[b];
    bool stop = (b >= 1 && (asmList[b - 1].opFlags &
                            OpFlag::OP_FLAG_IS_BRANCH)) ||
                checkAsmBackwardDep(asm_, asmPrev);
    if (stop) break;
    res.push_back(b);
  }

  return res;
}

void asmScanDeps(AsmFunc &func) {
  for (size_t i = 0; i < func.asm_.size(); ++i) {
    auto indices = asmGetReorderIndices(func.asm_, i);
    if (indices.empty()) continue;
    int min = *std::min_element(indices.begin(), indices.end());
    int max = *std::max_element(indices.begin(), indices.end());
    func.asm_[i].debug.reorderLineMin =
        (min >= 0 && min < (int)func.asm_.size())
            ? func.asm_[min].debug.lineASM
            : 0;
    func.asm_[i].debug.reorderLineMax =
        (max >= 0 && max < (int)func.asm_.size())
            ? func.asm_[max].debug.lineASM
            : 0;
  }
}

} // namespace rspl

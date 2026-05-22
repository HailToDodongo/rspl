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

// --- Register index lookup (295 entries, O(1) without hashing) ---------

// Scalar register order: $zero..$ra (standard MIPS ABI, $at excluded)
// Must match the order in the REG_INDEX_MAP (256..287).
static int scalarRegIdx(const char *s) {
  if (s[0] == '$') ++s; else return -1;
  if (s[0] == 'z' && s[1] == 'e') return 0;   // $zero
  if (s[0] == 'a' && s[1] == 't') return 1;   // $at
  if (s[0] == 'v' && s[1] >= '0' && s[1] <= '1') return 2 + (s[1]-'0'); // $v0-1
  if (s[0] == 'a' && s[1] >= '0' && s[1] <= '3') return 4 + (s[1]-'0'); // $a0-3
  if (s[0] == 't' && s[1] >= '0' && s[1] <= '7') return 8 + (s[1]-'0'); // $t0-7
  if (s[0] == 's' && s[1] >= '0' && s[1] <= '7') return 16 + (s[1]-'0'); // $s0-7
  if (s[0] == 't' && s[1] == '8') return 24;  // $t8
  if (s[0] == 't' && s[1] == '9') return 25;  // $t9
  if (s[0] == 'k' && s[1] == '0') return 26;  // $k0
  if (s[0] == 'k' && s[1] == '1') return 27;  // $k1
  if (s[0] == 'g' && s[1] == 'p') return 28;  // $gp
  if (s[0] == 's' && s[1] == 'p') return 29;  // $sp
  if (s[0] == 'f' && s[1] == 'p') return 30;  // $fp
  if (s[0] == 'r' && s[1] == 'a') return 31;  // $ra
  return -1;
}

// Fast register → index lookup. Returns -1 if not found.
int getRegIndex(const std::string &name) {
  if (name.size() < 3 || name[0] != '$') return -1;

  // Vector register: $vNN or $vNN_L (must have two digits after 'v',
  // distinguishing from scalar $v0/$v1 which have only one digit).
  if (name[1] == 'v' && name.size() >= 4 && name[2] >= '0' && name[2] <= '9' &&
      name[3] >= '0' && name[3] <= '9') {
    int vnum = (name[2] - '0') * 10 + (name[3] - '0');
    if (vnum >= 32) return -1;
    int base = vnum * 8;
    auto uscore = name.find('_', 4);
    if (uscore == std::string::npos) return base;
    int lane = name[uscore + 1] - '0';
    if (lane >= 0 && lane < 8) return base + lane;
    return base;
  }

  // Scalar: $zero..$ra
  int si = scalarRegIdx(name.c_str());
  if (si >= 0) return 256 + si;

  // Special registers
  if (name == "$vco") return 288;
  if (name == "$vcc") return 289;
  if (name == "$acc") return 290;
  if (name == "$divOut") return 291;
  if (name == "$divIn") return 292;
  if (name == "$divDP") return 293;
  if (name == "$vce") return 294;

  return -1;
}

// --- Stall index lookup (64 entries) -----------------------------------

int getRegStallIndex(const std::string &name) {
  return getRegStallIndex(name.c_str(), name.size());
}

int getRegStallIndex(const char *name, size_t len) {
  if (len < 3 || name[0] != '$') return -1;

  // Vector register: $vNN (two digits after 'v')
  if (name[1] == 'v' && len >= 4 && name[2] >= '0' && name[2] <= '9' &&
      name[3] >= '0' && name[3] <= '9') {
    int vnum = (name[2] - '0') * 10 + (name[3] - '0');
    if (vnum >= 32) return -1;
    return 32 + vnum;
  }

  // Scalar
  int si = scalarRegIdx(name);
  if (si >= 0) return si;
  return -1;
}

// --- Hidden registers -------------------------------------------------

const std::unordered_map<Opcode, std::vector<std::string>>
    HIDDEN_REGS_READ = []() {
      std::unordered_map<Opcode, std::vector<std::string>> m;
      m[getOpcode("vlt")] = {"$vco"};
      m[getOpcode("veq")] = {"$vco"};
      m[getOpcode("vne")] = {"$vco"};
      m[getOpcode("vge")] = {"$vco"};
      m[getOpcode("vmrg")] = {"$vcc"};
      m[getOpcode("vcl")] = {"$vco", "$vce"};
      m[getOpcode("vmacf")] = {"$acc"};
      m[getOpcode("vmacu")] = {"$acc"};
      m[getOpcode("vmadn")] = {"$acc"};
      m[getOpcode("vmadl")] = {"$acc"};
      m[getOpcode("vmadm")] = {"$acc"};
      m[getOpcode("vmadh")] = {"$acc"};
      m[getOpcode("vrndp")] = {"$acc"};
      m[getOpcode("vrndn")] = {"$acc"};
      m[getOpcode("vmacq")] = {"$acc"};
      m[getOpcode("vsar")] = {"$acc"};
      m[getOpcode("vrcph")] = {"$divOut"};
      m[getOpcode("vrsqh")] = {"$divOut"};
      m[getOpcode("vrcpl")] = {"$divIn", "$divDP"};
      m[getOpcode("vrsql")] = {"$divIn", "$divDP"};
      m[getOpcode("vadd")] = {"$vco"};
      m[getOpcode("vsub")] = {"$vco"};
      return m;
    }();

const std::unordered_map<Opcode, std::vector<std::string>>
    HIDDEN_REGS_WRITE = []() {
      std::unordered_map<Opcode, std::vector<std::string>> m;
      m[getOpcode("vlt")] = {"$vcc", "$vco", "$acc"};
      m[getOpcode("veq")] = {"$vcc", "$vco", "$acc"};
      m[getOpcode("vne")] = {"$vcc", "$vco", "$acc"};
      m[getOpcode("vge")] = {"$vcc", "$vco", "$acc"};
      m[getOpcode("vch")] = {"$vcc", "$vco", "$acc", "$vce"};
      m[getOpcode("vcr")] = {"$vcc", "$vco", "$acc", "$vce"};
      m[getOpcode("vcl")] = {"$vcc", "$vco", "$acc", "$vce"};
      m[getOpcode("vmrg")] = {"$vco", "$acc"};
      m[getOpcode("vmov")] = {"$acc"};
      m[getOpcode("vrcp")] = {"$acc", "$divOut", "$divDP"};
      m[getOpcode("vrcph")] = {"$acc", "$divIn", "$divDP"};
      m[getOpcode("vrcpl")] = {"$acc", "$divOut", "$divDP"};
      m[getOpcode("vrsq")] = {"$acc", "$divOut", "$divDP"};
      m[getOpcode("vrsqh")] = {"$acc", "$divIn", "$divDP"};
      m[getOpcode("vrsql")] = {"$acc", "$divOut", "$divDP"};
      m[getOpcode("vadd")] = {"$vco", "$acc"};
      m[getOpcode("vsub")] = {"$vco", "$acc"};
      m[getOpcode("vaddc")] = {"$vco", "$acc"};
      m[getOpcode("vsubc")] = {"$vco", "$acc"};
      m[getOpcode("vabs")] = {"$acc"};
      m[getOpcode("vand")] = {"$acc"};
      m[getOpcode("vnand")] = {"$acc"};
      m[getOpcode("vor")] = {"$acc"};
      m[getOpcode("vnor")] = {"$acc"};
      m[getOpcode("vxor")] = {"$acc"};
      m[getOpcode("vnxor")] = {"$acc"};
      m[getOpcode("vmulf")] = {"$acc"};
      m[getOpcode("vmulu")] = {"$acc"};
      m[getOpcode("vmacf")] = {"$acc"};
      m[getOpcode("vmacu")] = {"$acc"};
      m[getOpcode("vmudn")] = {"$acc"};
      m[getOpcode("vmadn")] = {"$acc"};
      m[getOpcode("vmudl")] = {"$acc"};
      m[getOpcode("vmadl")] = {"$acc"};
      m[getOpcode("vmudm")] = {"$acc"};
      m[getOpcode("vmadm")] = {"$acc"};
      m[getOpcode("vmudh")] = {"$acc"};
      m[getOpcode("vmadh")] = {"$acc"};
      m[getOpcode("vrndp")] = {"$acc"};
      m[getOpcode("vrndn")] = {"$acc"};
      m[getOpcode("vmulq")] = {"$acc"};
      m[getOpcode("vmacq")] = {"$acc"};
      return m;
    }();

static const std::unordered_set<std::string> STALL_IGNORE_REGS = {
    "$vcc", "$vco", "$acc", "$vce", "$divOut", "$divIn", "$divDP"};

static const std::unordered_set<Opcode> READ_ONLY_OPS = []() {
  std::unordered_set<Opcode> s;
  for (auto *op : {"beq","bne","bgezal","bltzal","bgez","bltz",
       "blez","bgtz","j","jr","jal",
       "sw","sh","sb","sbv","ssv","slv","sdv",
       "sqv","spv","suv","shv","sfv","stv","swv","srv",
       "mtc0"})
    s.insert(getOpcode(op));
  return s;
}();

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

// --- Precomputed register expansion cache ------------------------------

namespace {
struct ExpandCache {
  std::unordered_map<std::string, std::vector<std::string>> map;
  ExpandCache() {
    static const std::unordered_map<std::string, std::vector<int>> laneMap = {
        {"v",{0,1,2,3,4,5,6,7}},{".q0",{0,2,4,6}},{".q1",{1,3,5,7}},
        {".h0",{0,4}},{".h1",{1,5}},{".h2",{2,6}},{".h3",{3,7}},
        {".e0",{0}},{".e1",{1}},{".e2",{2}},{".e3",{3}},
        {".e4",{4}},{".e5",{5}},{".e6",{6}},{".e7",{7}},
    };
    const char *vecs[] = {"$v00","$v01","$v02","$v03","$v04","$v05","$v06",
      "$v07","$v08","$v09","$v10","$v11","$v12","$v13","$v14","$v15",
      "$v16","$v17","$v18","$v19","$v20","$v21","$v22","$v23",
      "$v24","$v25","$v26","$v27","$v28","$v29","$v30","$v31"};
    for (auto *v : vecs) {
      std::string reg(v);
      auto &full = map[reg];
      for (int l = 0; l < 8; ++l) full.push_back(reg + "_" + std::to_string(l));
      for (auto &[ln, lanes] : laneMap) {
        std::string key = reg + (ln[0] == '.' ? ln : "");
        if (ln == "v") { map[key] = full; continue; }
        for (int l : lanes) map[key].push_back(reg + "_" + std::to_string(l));
      }
    }
    const char *scalars[] = {"$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3",
      "$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7",
      "$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7",
      "$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra",
      "$vcc","$vco","$vce","$acc","$divOut","$divIn","$divDP"};
    for (auto *s : scalars) map[s] = {s};
  }
};
} // namespace

const std::vector<std::string> &expandRegister(const std::string &regName) {
  static const ExpandCache cache;
  auto it = cache.map.find(regName);
  if (it != cache.map.end()) return it->second;

  // Fallback: swizzle patterns not in cache (e.g. $v01.xyzwXYZW → .v)
  auto dotPos = regName.find('.');
  if (dotPos != std::string::npos) {
    std::string reg = regName.substr(0, dotPos);
    if (reg::isVecReg(reg)) {
      auto sit = SWIZZLE_MAP.find(regName.substr(dotPos + 1));
      if (sit != SWIZZLE_MAP.end()) {
        auto rit = cache.map.find(reg + sit->second);
        if (rit != cache.map.end()) return rit->second;
      }
    }
  }
  static thread_local std::vector<std::string> t_fallback;
  t_fallback.clear();
  t_fallback.push_back(regName);
  return t_fallback;
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
  if (inst.op == Op::JR() || inst.op == Op::MTC2() ||
      inst.op == Op::MTC0() || inst.op == Op::CTC2()) {
    return {inst.args[0]};
  }
  if ((inst.opFlags & OpFlag::OP_FLAG_IS_BRANCH) &&
      getOpcodeName(inst.op)[0] == 'b') {
    if (inst.args.empty()) return {};
    return std::vector<std::string>(inst.args.begin(),
                                    inst.args.end() - 1);
  }
  if (inst.opFlags & OpFlag::OP_FLAG_IS_STORE) {
    if (inst.op == Op::STV()) {
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
  if (inst.op == Op::J() || inst.op == Op::JAL()) {
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
      inst.op == Op::LTV()) {
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
      (inst.op == Op::MTC2() || inst.op == Op::CTC2()) ? inst.args[1]
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
  // Clear all dependency data
  inst.depsSourceIdx.clear();
  inst.depsTargetIdx.clear();
  inst.depsStallSourceIdx.clear();
  inst.depsStallTargetIdx.clear();
  inst.depsSourceMask = {};
  inst.depsTargetMask = {};
  inst.depsStallSourceMask0 = 0;
  inst.depsStallSourceMask1 = 0;
  inst.depsStallTargetMask0 = 0;
  inst.depsStallTargetMask1 = 0;
  inst.barrierMask = 0;

  if (inst.type != AsmType::OP || (inst.opFlags & OpFlag::OP_FLAG_IS_NOP))
    return;

  // Scratch dedup: up to 16 unique raw regs before expansion, ≤ 24 after.
  // Using a stack-allocated bool array indexed by register number instead
  // of unordered_set<string> avoids all intermediate heap allocations.
  bool seenBase[64] = {}; // for raw base reg dedup (max 64 stall indices)

  // --- Source registers: expand → mask + idx, track bases for stalls ----

  auto rawSrc = getSourceRegs(inst);
  for (auto &r : rawSrc) {
    std::string ext = extractRegFromArg(r);
    if (ext.empty() || ext[0] != '$') continue;

    // Track base register for stall info (deduplicated)
    auto dotPos = ext.find('.');
    int baseLen = (dotPos != std::string::npos) ? (int)dotPos : (int)ext.size();
    // Use a cheap inline comparison for STALL_IGNORE_REGS
    bool isStallIgnored =
        (baseLen == 4 && ext[1] == 'v' && ext[2] == 'c' && ext[3] == 'c') ||
        (baseLen == 4 && ext[1] == 'v' && ext[2] == 'c' && ext[3] == 'o') ||
        (baseLen == 4 && ext[1] == 'v' && ext[2] == 'c' && ext[3] == 'e') ||
        (baseLen == 4 && ext[1] == 'a' && ext[2] == 'c' && ext[3] == 'c') ||
        (baseLen == 6 && ext == "$divOut") ||
        (baseLen == 5 && ext == "$divIn") ||
        (baseLen == 5 && ext == "$divDP");
    if (!isStallIgnored) {
      int stallIdx = getRegStallIndex(ext.c_str(), baseLen);
      if (stallIdx >= 0 && !seenBase[stallIdx]) {
        seenBase[stallIdx] = true;
        inst.depsStallSourceIdx.push_back(stallIdx);
      }
    }

    // Expand raw reg to lanes, set mask + index for each
    const auto &expanded = expandRegister(ext);
    for (const auto &e : expanded) {
      int idx = getRegIndex(e);
      if (idx >= 0) {
        inst.depsSourceMask[idx / 64] |= (1ULL << (idx % 64));
        inst.depsSourceIdx.push_back(idx);
      }
    }
  }

  // --- Target registers: expand → mask + idx, track bases for stalls ---

  // Reuse seenBase for target stall dedup — clear first
  for (int &s : inst.depsStallSourceIdx) seenBase[s] = false;

  auto rawTgt = getTargetRegs(inst);
  for (auto &r : rawTgt) {
    auto dotPos = r.find('.');
    int baseLen = (dotPos != std::string::npos) ? (int)dotPos : (int)r.size();
    bool isStallIgnored =
        (baseLen == 4 && r[1] == 'v' && r[2] == 'c' && r[3] == 'c') ||
        (baseLen == 4 && r[1] == 'v' && r[2] == 'c' && r[3] == 'o') ||
        (baseLen == 4 && r[1] == 'v' && r[2] == 'c' && r[3] == 'e') ||
        (baseLen == 4 && r[1] == 'a' && r[2] == 'c' && r[3] == 'c') ||
        (baseLen == 6 && r == "$divOut") ||
        (baseLen == 5 && r == "$divIn") ||
        (baseLen == 5 && r == "$divDP");
    if (!isStallIgnored) {
      int stallIdx = getRegStallIndex(r.c_str(), baseLen);
      if (stallIdx >= 0 && !seenBase[stallIdx]) {
        seenBase[stallIdx] = true;
        inst.depsStallTargetIdx.push_back(stallIdx);
      }
    }

    const auto &expanded = expandRegister(r);
    for (const auto &e : expanded) {
      int idx = getRegIndex(e);
      if (idx >= 0) {
        inst.depsTargetMask[idx / 64] |= (1ULL << (idx % 64));
        inst.depsTargetIdx.push_back(idx);
      }
    }
  }

  // --- Stall masks from the idx vectors (already built inline above) ------

  uint64_t srcStallMask = 0, tgtStallMask = 0;
  for (int idx : inst.depsStallSourceIdx) srcStallMask |= (1ULL << idx);
  for (int idx : inst.depsStallTargetIdx) tgtStallMask |= (1ULL << idx);
  inst.depsStallSourceMask0 = static_cast<uint32_t>(srcStallMask);
  inst.depsStallSourceMask1 = static_cast<uint32_t>(srcStallMask >> 32);
  inst.depsStallTargetMask0 = static_cast<uint32_t>(tgtStallMask);
  inst.depsStallTargetMask1 = static_cast<uint32_t>(tgtStallMask >> 32);

  // Barrier mask from annotations
  for (auto &ann : inst.cold->annotations) {
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

static bool maskGetBit(const RegMask &m, int idx) {
  return (m[idx / 64] >> (idx % 64)) & 1ULL;
}

static void maskSetBit(RegMask &m, int idx) {
  m[idx / 64] |= (1ULL << (idx % 64));
}

std::vector<int> asmGetReorderIndices(const std::vector<AsmInst> &asmList,
                                      int i) {
  const AsmInst &asm_ = asmList[i];
  if (asm_.opFlags & OpFlag::OP_FLAG_IS_IMMOVABLE) return {i};

  int lastWrite[REG_INDEX_SIZE] = {};
  RegMask lastWriteMask = {};
  RegMask lastReadMask = {};

  int pos = static_cast<int>(asmList.size());

  // --- Scan forward ---
  bool isPastBranch = false;
  int f;
  for (f = i + 1; f < (int)asmList.size(); ++f) {
    const AsmInst &asmNext = asmList[f];
    const AsmInst *prevPrev =
        (f >= 2) ? &asmList[f - 2] : nullptr;

    bool isFilledBranch =
        (asmNext.opFlags & OpFlag::OP_FLAG_IS_BRANCH) &&
        !((f + 1 < (int)asmList.size()) &&
          (asmList[f + 1].opFlags & OpFlag::OP_FLAG_IS_NOP));
    isPastBranch =
        prevPrev && (prevPrev->opFlags & OpFlag::OP_FLAG_IS_BRANCH);

    if (isFilledBranch || isPastBranch ||
        checkAsmBackwardDep(asmNext, asm_)) {
      pos = f;
      break;
    }

    // Track last writes for each register
    for (int reg : asmNext.depsTargetIdx) {
      lastWrite[reg] = f;
      maskSetBit(lastWriteMask, reg);
    }
  }

  // --- Second pass: collect reads after stop point ---
  int fRead = isPastBranch ? f - 2 : f;
  for (; fRead < (int)asmList.size(); ++fRead) {
    for (int idx = 0; idx < 5; ++idx)
      lastReadMask[idx] |= asmList[fRead].depsSourceMask[idx];
    if (asmList[fRead].opFlags & OpFlag::OP_FLAG_IS_BRANCH) break;
  }

  // --- Check read-after-write across the gap ---
  for (int reg : asm_.depsTargetIdx) {
    int lastWritePos = lastWrite[reg];
    if (lastWritePos && maskGetBit(lastReadMask, reg)) {
      pos = std::min(lastWritePos, pos);
    }
  }

  // --- Build forward range ---
  std::vector<int> res;
  for (int r = i; r <= pos - 1; ++r) res.push_back(r);

  // --- Collect registers not overwritten after us ---
  RegMask writeCheckRegsMask = asm_.depsTargetMask;
  for (int idx = 0; idx < 5; ++idx)
    writeCheckRegsMask[idx] &= ~lastWriteMask[idx];

  // --- Scan backward ---
  for (int b = i - 1; b >= 0; --b) {
    const AsmInst &asmPrev = asmList[b];
    bool stop = (b >= 1 && (asmList[b - 1].opFlags &
                            OpFlag::OP_FLAG_IS_BRANCH)) ||
                checkAsmBackwardDep(asm_, asmPrev) ||
                maskAnd(asmPrev.depsTargetMask, writeCheckRegsMask);
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

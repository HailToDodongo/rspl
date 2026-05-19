#include "registers.h"

#include <algorithm>

namespace rspl::reg {

const std::vector<std::string> REGS_SCALAR = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
};

const std::vector<std::string> REGS_VECTOR = {
    "$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07",
    "$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15",
    "$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23",
    "$v24", "$v25", "$v26", "$v27", "$v28", "$v29", "$v30", "$v31",
};

const std::vector<std::string> REGS_ALLOC_SCALAR = {
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
    "$k0", "$k1", "$sp", "$fp",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
};

const std::vector<std::string> REGS_ALLOC_VECTOR = {
    "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07",
    "$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15",
    "$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23",
    "$v24", "$v25", "$v26", "$v27", "$v28",
};

const std::vector<std::string> REGS_FORBIDDEN = {
    Reg::AT, Reg::GP, Reg::VTEMP0,
};

bool isVecReg(const std::string &regName) {
  return std::find(REGS_VECTOR.begin(), REGS_VECTOR.end(), regName) !=
         REGS_VECTOR.end();
}

static int indexIn(const std::string &name,
                   const std::vector<std::string> &list) {
  for (size_t i = 0; i < list.size(); ++i) {
    if (list[i] == name) return static_cast<int>(i);
  }
  return -1;
}

const std::string *nextReg(const std::string &regName, int offset) {
  int idx = indexIn(regName, REGS_VECTOR);
  if (idx >= 0 && idx + offset < (int)REGS_VECTOR.size()) {
    return &REGS_VECTOR[idx + offset];
  }
  idx = indexIn(regName, REGS_SCALAR);
  if (idx >= 0 && idx + offset < (int)REGS_SCALAR.size()) {
    return &REGS_SCALAR[idx + offset];
  }
  return nullptr;
}

const std::string *nextVecReg(const std::string &regName) {
  int idx = indexIn(regName, REGS_VECTOR);
  if (idx >= 0 && idx + 1 < (int)REGS_VECTOR.size()) {
    return &REGS_VECTOR[idx + 1];
  }
  return nullptr;
}

} // namespace rspl::reg

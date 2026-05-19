#pragma once

#include <string>
#include <vector>

namespace rspl::reg {

// --- Register name constants ------------------------------------------

struct Reg {
  static constexpr const char *AT = "$at";
  static constexpr const char *ZERO = "$zero";
  static constexpr const char *V0 = "$v0";
  static constexpr const char *V1 = "$v1";
  static constexpr const char *A0 = "$a0";
  static constexpr const char *A1 = "$a1";
  static constexpr const char *A2 = "$a2";
  static constexpr const char *A3 = "$a3";
  static constexpr const char *T0 = "$t0";
  static constexpr const char *T1 = "$t1";
  static constexpr const char *T2 = "$t2";
  static constexpr const char *T3 = "$t3";
  static constexpr const char *T4 = "$t4";
  static constexpr const char *T5 = "$t5";
  static constexpr const char *T6 = "$t6";
  static constexpr const char *T7 = "$t7";
  static constexpr const char *T8 = "$t8";
  static constexpr const char *T9 = "$t9";
  static constexpr const char *S0 = "$s0";
  static constexpr const char *S1 = "$s1";
  static constexpr const char *S2 = "$s2";
  static constexpr const char *S3 = "$s3";
  static constexpr const char *S4 = "$s4";
  static constexpr const char *S5 = "$s5";
  static constexpr const char *S6 = "$s6";
  static constexpr const char *S7 = "$s7";
  static constexpr const char *K0 = "$k0";
  static constexpr const char *K1 = "$k1";
  static constexpr const char *GP = "$gp";
  static constexpr const char *SP = "$sp";
  static constexpr const char *FP = "$fp";
  static constexpr const char *RA = "$ra";

  static constexpr const char *V00 = "$v00";
  static constexpr const char *V01 = "$v01";
  static constexpr const char *V02 = "$v02";
  static constexpr const char *V03 = "$v03";
  static constexpr const char *V04 = "$v04";
  static constexpr const char *V05 = "$v05";
  static constexpr const char *V06 = "$v06";
  static constexpr const char *V07 = "$v07";
  static constexpr const char *V08 = "$v08";
  static constexpr const char *V09 = "$v09";
  static constexpr const char *V10 = "$v10";
  static constexpr const char *V11 = "$v11";
  static constexpr const char *V12 = "$v12";
  static constexpr const char *V13 = "$v13";
  static constexpr const char *V14 = "$v14";
  static constexpr const char *V15 = "$v15";
  static constexpr const char *V16 = "$v16";
  static constexpr const char *V17 = "$v17";
  static constexpr const char *V18 = "$v18";
  static constexpr const char *V19 = "$v19";
  static constexpr const char *V20 = "$v20";
  static constexpr const char *V21 = "$v21";
  static constexpr const char *V22 = "$v22";
  static constexpr const char *V23 = "$v23";
  static constexpr const char *V24 = "$v24";
  static constexpr const char *V25 = "$v25";
  static constexpr const char *V26 = "$v26";
  static constexpr const char *V27 = "$v27";
  static constexpr const char *V28 = "$v28";
  static constexpr const char *V29 = "$v29";
  static constexpr const char *V30 = "$v30";
  static constexpr const char *V31 = "$v31";

  static constexpr const char *VZERO = "$v00";
  static constexpr const char *VTEMP0 = "$v29";
  static constexpr const char *VSHIFT = "$v30";
  static constexpr const char *VSHIFT8 = "$v31";
};

struct RegCop0 {
  static constexpr const char *DMA_BUSY = "COP0_DMA_BUSY";
  static constexpr const char *DP_START = "COP0_DP_START";
  static constexpr const char *DP_END = "COP0_DP_END";
  static constexpr const char *DP_CURRENT = "COP0_DP_CURRENT";
  static constexpr const char *DP_CLOCK = "COP0_DP_CLOCK";
  static constexpr const char *DMA_SPADDR = "COP0_DMA_SPADDR";
  static constexpr const char *DMA_RAMADDR = "COP0_DMA_RAMADDR";
  static constexpr const char *DMA_READ = "COP0_DMA_READ";
  static constexpr const char *DMA_WRITE = "COP0_DMA_WRITE";
  static constexpr const char *SP_STATUS = "COP0_SP_STATUS";
  static constexpr const char *DMA_FULL = "COP0_DMA_FULL";
};

struct RegCop2 {
  static constexpr const char *VCO = "$vc0";
  static constexpr const char *VCC = "$vcc";
  static constexpr const char *VCE = "$vce";
  static constexpr const char *ACC_MD = "COP2_ACC_MD";
  static constexpr const char *ACC_HI = "COP2_ACC_HI";
  static constexpr const char *ACC_LO = "COP2_ACC_LO";
};

// --- Register lists ---------------------------------------------------

extern const std::vector<std::string> REGS_SCALAR;
extern const std::vector<std::string> REGS_VECTOR;
extern const std::vector<std::string> REGS_ALLOC_SCALAR;
extern const std::vector<std::string> REGS_ALLOC_VECTOR;
extern const std::vector<std::string> REGS_FORBIDDEN;

// --- Register helpers -------------------------------------------------

bool isVecReg(const std::string &regName);
const std::string *nextReg(const std::string &regName, int offset = 1);
const std::string *nextVecReg(const std::string &regName);

} // namespace rspl::reg

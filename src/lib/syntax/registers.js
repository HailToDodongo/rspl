/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

export const REG = {
  AT: "$at", ZERO: "$zero",
  V0: "$v0", V1: "$v1",
  A0: "$a0", A1: "$a1", A2: "$a2", A3: "$a3",
  T0: "$t0", T1: "$t1", T2: "$t2", T3: "$t3", T4: "$t4", T5: "$t5", T6: "$t6", T7: "$t7", T8: "$t8", T9: "$t9",
  S0: "$s0", S1: "$s1", S2: "$s2", S3: "$s3", S4: "$s4", S5: "$s5", S6: "$s6", S7: "$s7",
  K0: "$k0", K1: "$k1",
  GP: "$gp", SP: "$sp", FP: "$fp", RA: "$ra",
  V00: "$v00", V01: "$v01", V02: "$v02", V03: "$v03", V04: "$v04", V05: "$v05", V06: "$v06", V07: "$v07",
  V08: "$v08", V09: "$v09", V10: "$v10", V11: "$v11", V12: "$v12", V13: "$v13", V14: "$v14", V15: "$v15",
  V16: "$v16", V17: "$v17", V18: "$v18", V19: "$v19", V20: "$v20", V21: "$v21", V22: "$v22", V23: "$v23",
  V24: "$v24", V25: "$v25", V26: "$v26", V27: "$v27", V28: "$v28", V29: "$v29", V30: "$v30", V31: "$v31",
};

export const REGS_SCALAR = [
  "$at", "$zero", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
  "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
  "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
  "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
];

export const REGS_VECTOR = [
  "$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07",
  "$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15",
  "$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23",
  "$v24", "$v25", "$v26", "$v27", "$v28", "$v29", "$v30", "$v31",
];

export function isVecReg(regName) {
  return REGS_VECTOR.includes(regName);
}

export function nextVecReg(regName) {
  const idx = REGS_VECTOR.indexOf(regName);
  return idx === -1 ? null : REGS_VECTOR[idx+1];
}

export function nextReg(regName, offset) {
  let idx = REGS_VECTOR.indexOf(regName);
  if(idx >= 0) {
    return REGS_VECTOR[idx+offset];
  }
  idx = REGS_SCALAR.indexOf(regName);
  if(idx >= 0) {
    return REGS_SCALAR[idx+offset];
  }
  return null;
}

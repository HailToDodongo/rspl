/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

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

export function nextVecReg(regName) {
  const idx = REGS_VECTOR.indexOf(regName);
  return idx === -1 ? null : REGS_VECTOR[idx+1];
}
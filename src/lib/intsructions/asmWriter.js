/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import state from "../state.js";
import {
  BRANCH_OPS,
  IMMOVABLE_OPS,
  LOAD_OPS,
  LOAD_OPS_SCALAR,
  LOAD_OPS_VECTOR,
  MEM_STALL_LOAD_OPS,
  MEM_STALL_STORE_OPS, OP_FLAG_CTC2_CFC2,
  OP_FLAG_IS_BRANCH,
  OP_FLAG_IS_IMMOVABLE, OP_FLAG_IS_LIKELY,
  OP_FLAG_IS_LOAD,
  OP_FLAG_IS_MEM_STALL_LOAD, OP_FLAG_IS_MEM_STALL_STORE, OP_FLAG_IS_NOP,
  OP_FLAG_IS_STORE, OP_FLAG_IS_VECTOR, OP_FLAG_LIKELY_BRANCH,
  STORE_OPS
} from "../optimizer/asmScanDeps.js";
import {REG} from "../syntax/registers.js";
import {getAnnotationVal} from "../syntax/annotations.js";

export const ASM_TYPE = {
  OP: 0,
  LABEL: 1,
//  COMMENT: 2,
  INLINE: 3,
}

/** @param {string} op */
function getStallLatency(op) {
  if(op.startsWith("v") || op === "mtc2")return 4;
  if(LOAD_OPS_VECTOR.includes(op))return 4;
  if(LOAD_OPS_SCALAR.includes(op))return 3;
  if(["mfc0", "mfc2", "cfc2", "ctc2"].includes(op))return 3;
  return 0;
}

/**
 * @returns {ASMDebug}
 */
function getDebugData() {
  return {
    lineASM: 0,
    lineRSPL: state.line,
    lineASMOpt: 0,
    reorderCount: 0,
    reorderLineMin: 0,
    reorderLineMax: 0,
    cycle: 0,
  };
}

/**
 *
 * @param op
 * @return {ASM}
 */
function getOpInfo(op) {
  const annotations = state.getAnnotations();
  const res = {
    opFlags: (LOAD_OPS.includes(op) ? OP_FLAG_IS_LOAD : 0)
     | (STORE_OPS.includes(op) ? OP_FLAG_IS_STORE : 0)
     | (BRANCH_OPS.includes(op) ? OP_FLAG_IS_BRANCH : 0)
     | (IMMOVABLE_OPS.includes(op) ? OP_FLAG_IS_IMMOVABLE : 0)
     | (MEM_STALL_LOAD_OPS.includes(op) ? OP_FLAG_IS_MEM_STALL_LOAD : 0)
     | (MEM_STALL_STORE_OPS.includes(op) ? OP_FLAG_IS_MEM_STALL_STORE : 0)
     | (op.startsWith("v") ? OP_FLAG_IS_VECTOR : 0)
     | (op === "nop" ? OP_FLAG_IS_NOP : 0)
     | (!getAnnotationVal(annotations, "Unlikely") ? OP_FLAG_IS_LIKELY : 0)
     | ((op === "cfc2" || op === "ctc2") ? OP_FLAG_CTC2_CFC2 : 0)
    ,
    stallLatency: getStallLatency(op),
    annotations,
    depsArgMask: 0n,
  };
  if((res.opFlags & OP_FLAG_IS_BRANCH) && (res.opFlags & OP_FLAG_IS_LIKELY)) {
    res.opFlags |= OP_FLAG_LIKELY_BRANCH;
  }

  return res;
}

/**
 * Emits an assembly instruction
 * @param {string} op
 * @param {Array<string|number>} args
 * @return {ASM}
 */
export function asm(op, args) {
  return {type: ASM_TYPE.OP, op, args, debug: getDebugData(), ...getOpInfo(op)};
}

/**
 * Emits a function call
 * @param target
 * @param argRegs
 * @param {boolean} relative
 * @return {ASM}
 */
export function asmFunction(target, argRegs, relative = false) {
  return relative ? {
    type: ASM_TYPE.OP, op: "bgezal", args: [REG.ZERO, target],
    debug: getDebugData(), ...getOpInfo("bgezal"),
    funcArgs: argRegs
  } : {
    type: ASM_TYPE.OP, op: "jal", args: [target],
    debug: getDebugData(), ...getOpInfo("jal"),
    funcArgs: argRegs
  };
}

export function asmBranch(op, args, labelEnd) {
  return {type: ASM_TYPE.OP, op, args, debug: getDebugData(), ...getOpInfo(op), labelEnd};
}

export function asmInline(op, args = []) {
  return {type: ASM_TYPE.INLINE, op, args, debug: getDebugData(), ...getOpInfo(op)};
}

/** @returns {ASM} */
export function asmNOP() {
  return {type: ASM_TYPE.OP, op: "nop", args: [], debug: getDebugData(),
    ...getOpInfo("nop"),
  };
}

/** @returns {ASM} */
export function asmLabel(label) {
  return {type: ASM_TYPE.LABEL, label, op: "", args: [], debug: getDebugData(),
    ...getOpInfo(""),
  };
}

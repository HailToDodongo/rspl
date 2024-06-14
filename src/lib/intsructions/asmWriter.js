/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import state from "../state.js";
import {
  BRANCH_OPS,
  IMMOVABLE_OPS,
  LOAD_OPS, LOAD_OPS_SCALAR, LOAD_OPS_VECTOR,
  MEM_STALL_LOAD_OPS,
  MEM_STALL_STORE_OPS,
  STORE_OPS
} from "../optimizer/asmScanDeps.js";
import {REG} from "../syntax/registers.js";

export const ASM_TYPE = {
  OP: 0,
  LABEL: 1,
  COMMENT: 2,
  INLINE: 3,
}

/** @param {string} op */
function getStallLatency(op) {
  if(op.startsWith("v") || op === "mtc2")return 4;
  if(LOAD_OPS_VECTOR.includes(op))return 4;
  if(LOAD_OPS_SCALAR.includes(op))return 3;
  if(["mfc0", "mfc2", "cfc2"].includes(op))return 3;
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

function getOpInfo(op) {
  return {
    opIsLoad: LOAD_OPS.includes(op),
    opIsStore: STORE_OPS.includes(op),
    opIsBranch: BRANCH_OPS.includes(op),
    opIsImmovable: IMMOVABLE_OPS.includes(op),
    opIsMemStallLoad: MEM_STALL_LOAD_OPS.includes(op),
    opIsMemStallStore: MEM_STALL_STORE_OPS.includes(op),
    opIsVector: op.startsWith("v"),
    stallLatency: getStallLatency(op),
    isNOP: op === "nop",
    annotations: state.getAnnotations(),
    funcArgs: [],
    depsArgMask: 0n,
  };
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

export function asmInline(op, args) {
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

/** @returns {ASM} */
export function asmComment(comment) {
  return {type: ASM_TYPE.COMMENT, comment, op: "", args: [], debug: getDebugData(),
    ...getOpInfo(""),
  };
}

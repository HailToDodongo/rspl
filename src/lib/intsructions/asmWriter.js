/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import state from "../state.js";

export const ASM_TYPE = {
  OP: 0,
  LABEL: 1,
  COMMENT: 2,
}

/**
 * @returns {ASMDebug}
 */
function getDebugData() {
  return {
    lineRSPL: state.line,
    lineASM: 0,
  };
}

/**
 * Emits an assembly instruction
 * @param {string} op
 * @param {Array<string|number>} args
 * @return {ASM}
 */
export function asm(op, args) {
  return {type: ASM_TYPE.OP, op, args, debug: getDebugData()};
}

/** @returns {ASM} */
export function asmNOP() {
  return {type: ASM_TYPE.OP, op: "nop", args: [], debug: getDebugData()};
}

/** @returns {ASM} */
export function asmLabel(label) {
  return {type: ASM_TYPE.LABEL, label, op: "", args: [], debug: getDebugData()};
}

/** @returns {ASM} */
export function asmComment(comment) {
  return {type: ASM_TYPE.COMMENT, comment, op: "", args: [], debug: getDebugData()};
}

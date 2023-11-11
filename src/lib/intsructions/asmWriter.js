/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

export const ASM_TYPE = {
  OP: 0,
  LABEL: 1,
  COMMENT: 2,
}

/** @returns {ASM} */
export function asm(op, args) {
  return {type: ASM_TYPE.OP, op, args};
}

/** @returns {ASM} */
export function asmNOP() {
  return {type: ASM_TYPE.OP, op: "nop", args: []};
}

/** @returns {ASM} */
export function asmLabel(label) {
  return {type: ASM_TYPE.LABEL, label, op: "", args: [],};
}

/** @returns {ASM} */
export function asmComment(comment) {
  return {type: ASM_TYPE.COMMENT, comment, op: "", args: [],};
}

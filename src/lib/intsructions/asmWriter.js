/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

export const ASM_TYPE = {
  OP: 0,
  LABEL: 1,
  COMMENT: 2,
}

export function asm(op, args) {
  return {type: ASM_TYPE.OP, op, args};
}

export function asmNOP(op, args) {
  return {type: ASM_TYPE.OP, op: "nop", args: []};
}
export function asmLabel(label) {
  return {type: ASM_TYPE.LABEL, label};
}

export function asmComment(comment) {
  return {type: ASM_TYPE.COMMENT, comment};
}

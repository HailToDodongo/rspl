/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../intsructions/asmWriter.js";

/**
 * Optimizes flat ASM before turning it into a tree.
 * @param {ASMFunc} asmFunc
 */
export function optimizeASM(asmFunc)
{
  // strip comments
  asmFunc.asm = asmFunc.asm.filter(line => line.type !== ASM_TYPE.COMMENT);

  // @TODO: pattern matching, e.g. tail-call optimization
}
/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {REG} from "./syntax/registers.js";
import {ASM_TYPE} from "./intsructions/asmWriter.js";
import {READ_ONLY_OPS} from "./optimizer/asmScanDeps.js";

/**
 * Normalizes ASM output of a function.
 *
 * This is different to optimizing, as it's necessary to produce valid ASM.
 * E.g. some operations use assignments to the zero-register to keep the logic simple,
 * this gets removed here.
 * @param {ASMFunc} asmFunc
 */
export function normalizeASM(asmFunc)
{
  /** @type {ASM[]} */ const asm = [];

  for(const line of asmFunc.asm)
  {
    if(line.type !== ASM_TYPE.OP
      || READ_ONLY_OPS.includes(line.op)
      || line.args.length === 0
    ) {
      asm.push(line);
      continue;
    }

    // ignore any write to the zero registers
    let targetReg = ["mtc2"].includes(line.op) ? line.args[1] : line.args[0];
    if(targetReg.startsWith(REG.VZERO) || targetReg.startsWith(REG.ZERO)) {
      continue;
    }

    asm.push(line);
  }
  asmFunc.asm = asm;
}
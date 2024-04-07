/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {BRANCH_OPS} from "../asmScanDeps.js";
import {REG} from "../../syntax/registers.js";

/**
 * Sequence merger, simplifies redundant sequences & sequences that can be overlapped.
 * @param {ASMFunc} asmFunc
 */
export function mergeSequence(asmFunc)
{
  const lines = asmFunc.asm;
  for(let i=0; i<lines.length; ++i)
  {
    const asm = lines[i];

    // consecutive sequence of sqrt(), last step against vzero can be combined
    // with the start of the next sequence
    if(asm.op === "vrsqh" && asm.args[1].startsWith(REG.VZERO+".")
      && lines[i+1]?.op === "vrsqh"
      && lines[i+2]?.op === "vrsql"
    ) {
      lines[i+1].args[0] = asm.args[0];
      lines.splice(i, 1);
      i-=1;
      continue;
    }

    if(asm.op === "vrcph" && asm.args[1].startsWith(REG.VZERO+".")
      && lines[i+1]?.op === "vrcph"
      && lines[i+2]?.op === "vrcpl"
    ) {
      lines[i+1].args[0] = asm.args[0];
      lines.splice(i, 1);
      i-=1;
      continue;
    }

    // indirect multiply by zero / 32-bit multiply with 0 fractional
    if(
      asm.op === "vxor" && asm.args[1] === REG.VZERO && asm.args[2]?.startsWith(REG.VZERO) && // a vector gets an explicit zero assigned...
      lines[i+1]?.op === "vmudl" // ...and is then used in a multiply
    ) {
      // check if the registers match...
      const targetReg = asm.args[0];
      if(lines[i+1].args[0] === targetReg && lines[i+1].args[1] === targetReg)
      {
        // ...then remove the zero assignment, and just use zero as the multiply operand
        lines[i+1].args[1] = REG.VZERO;
        lines.splice(i, 1);
        i-=1;
        continue;
      }
    }


  }
}
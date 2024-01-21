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

  }
}
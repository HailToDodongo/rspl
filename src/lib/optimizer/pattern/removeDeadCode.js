/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {BRANCH_OPS, OP_FLAG_IS_NOP} from "../asmScanDeps.js";
import {REG} from "../../syntax/registers.js";

/**
 * Removes dead-code, currently code at the end of a function if an early jump is present.
 * @param {ASMFunc} asmFunc
 */
export function removeDeadCode(asmFunc)
{
  const lines = asmFunc.asm;
  /*for(let i=0; i<lines.length; ++i)
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
  }*/

  // iterate instructions in reverse and see when we hit an unconditional jump
  // then remove all instructions after that
  let lastSafeIndex = -1;
  // a function always ends with a jump+delay, skip those
  for(let i=lines.length-1-2; i>=0; --i)
  {
    const asm = lines[i];

    if(["j", "jr"].includes(asm.op)) {
      lastSafeIndex = i; // incl. delay-slot
      break;
    }
    if(asm.opFlags & OP_FLAG_IS_NOP)continue;
    break;
  }

  if(lastSafeIndex < 0)return;
  lines.splice(lastSafeIndex+2);
}
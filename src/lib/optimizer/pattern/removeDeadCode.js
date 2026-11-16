/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {OP_FLAG_IS_NOP, OP_FLAG_UNCOND_JUMP} from "../asmScanDeps.js";

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

  // a function normally ends with a jump+delay, skip those
  let i = lines.length - 1;
  if(lines[i-1]?.opFlags & OP_FLAG_UNCOND_JUMP)i -= 2;

  // walk back over trailing NOPs, an unconditional jump before them makes the rest unreachable
  for(; i >= 0; --i)
  {
    const asm = lines[i];
    if(asm.type === ASM_TYPE.LABEL)return; // code after a label is reachable
    if(["j", "jr"].includes(asm.op)) {
      lines.splice(i+2); // keep the delay-slot
      return;
    }
    if(!(asm.opFlags & OP_FLAG_IS_NOP))return;
  }
}

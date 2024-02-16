/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {BRANCH_OPS} from "../asmScanDeps.js";

/**
 * If a branch has the form of: " if(cond)goto TARGET; "
 * It will result in:
 *   beq ... TEMP_LABEL  // inverted by default
 *   nop
 *   j TARGET
 *   nop
 *   TEMP_LABEL:
 *   ...
 *
 * We can invert the condition to avoid to jumps and a label.
 * @param {ASMFunc} asmFunc
 */
export function branchJump(asmFunc)
{
  const lines = asmFunc.asm;
  for(let i=0; i<lines.length; ++i)
  {
    const asm = lines[i];
    if(asm.op === "beq" || asm.op === "bne")
    {
      const labelTemp = asm.args[asm.args.length-1];
      if(lines[i+1]?.isNOP &&
         lines[i+2]?.op === "j"   &&
         lines[i+3]?.isNOP &&
         lines[i+4]?.label === labelTemp)
      {
        const labelTarget = lines[i+2].args[lines[i+2].args.length-1];
        // invert condition and patch label
        asm.op = asm.op === "beq" ? "bne" : "beq";
        asm.args[asm.args.length-1] = labelTarget;

        lines.splice(i+2, 3); // remove jump, delay-slot, temp-label
        i-=2;
      }
    }

  }
}
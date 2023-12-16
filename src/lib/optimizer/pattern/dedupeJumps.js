/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {BRANCH_OPS} from "../asmScanDeps.js";

/**
 * De-duplicates Jumps
 * This simplifies a chain of direct jumps (e.g. nested ifs) to a single jump.
 * If possible, the original jump label is even deleted.
 * @param {ASMFunc} asmFunc
 */
export function dedupeJumps(asmFunc)
{
  // check for jumps/branches to labels that directly point to a jump itself
  const labelReplace = [];
  for(let i=0; i<asmFunc.asm.length; ++i)
  {
    const asm = asmFunc.asm[i];
    if(asm.type === ASM_TYPE.LABEL) {
      const asmNext0 = asmFunc.asm[i+1];
      if(asmNext0?.op === "j") {
        // mark labels that need to be replaced
        labelReplace.push([asm.label, asmNext0.args[0]]);

        // additionally check if a jump precedes our label.
        // If that is the case we can remove the code itself since no one can reach it anymore.
        if(asmFunc.asm[i-2]?.op === "j") {
          asmFunc.asm.splice(i, 3); // remove label, jump, delay-slot (always a nop)
          --i;
        }
      }
    }
  }

  // replace labels in all jumps/branches
  for(const asm of asmFunc.asm) {
    if(BRANCH_OPS.includes(asm.op)) {
      const label = asm.args[asm.args.length-1];
      for(const [labelOld, labelNew] of labelReplace) {
        if(label === labelOld)asm.args[asm.args.length-1] = labelNew;
      }
    }
  }
}
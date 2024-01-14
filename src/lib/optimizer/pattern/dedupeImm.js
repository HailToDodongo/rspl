/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {REG} from "../../syntax/registers.js";
import {BRANCH_OPS, getTargetRegs} from "../asmScanDeps.js";

/**
 * De-duplicates immediate loads and labels
 * @param {ASMFunc} asmFunc
 */
export function dedupeImmediate(asmFunc)
{
  let lastAT = undefined;
  // ...now keep the first one, remove the others and patch the references
  const asmNew = [];
  for(const asm of asmFunc.asm)
  {
    let keep = true;
    if(asm.type === ASM_TYPE.OP)
    {
      if(BRANCH_OPS.includes(asm.op))lastAT = undefined;

      const targetRegs = getTargetRegs(asm);
      if(targetRegs.includes(REG.AT))
      {
        let newAT = undefined;
        // only handle "ori" for now, other instructions are assumed to set it in unknown ways
        if(asm.op === "ori") {
          newAT = asm.args[2];
          if(lastAT === newAT) {
            keep = false;
          }
        }
        lastAT = newAT;
      }
    } else {
      lastAT = undefined;
    }

    if(keep)asmNew.push(asm);
  }
  asmFunc.asm = asmNew;
}
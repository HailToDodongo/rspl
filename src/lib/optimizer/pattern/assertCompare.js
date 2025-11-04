/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {BRANCH_OPS, OP_FLAG_IS_BRANCH, OP_FLAG_IS_NOP} from "../asmScanDeps.js";
import {REG} from "../../syntax/registers.js";
import {invertBranchOp} from "../../operations/branch.js";
import {LABEL_ASSERT} from "../../builtins/libdragon.js";

/**
* Asserts can be inside an if-condition.
* This will lead to bad code since the inner content will set $at
* preventing the branches to be merged.
*
* E.g.: "if(x > 4)assert(0xAB);"
* becomes:
*   sltiu $at, $t0, 5
*   bne $at, $zero, LABEL_a_0001
*   nop
*   lui $at, 171
*   j assertion_failed
*   nop
*   LABEL_a_0001:
*
* But it should be merged into a single branch, pulling thr $at outside
* and inverting the branch
*
* e.g.:
*   lui $at, 171
*   beq $at, $zero, assertion_failed
 *  sltiu $at, $t0, 5
*
*
 * @param {ASMFunc} asmFunc
 */
export function assertCompare(asmFunc)
{
  const asm = asmFunc.asm;
  for(let i=0; i<asmFunc.asm.length-5; ++i)
  {
    if(((asm[i+0].opFlags & OP_FLAG_IS_BRANCH && asm[i+0].op.startsWith("b")))
      && (asm[i+1].opFlags & OP_FLAG_IS_NOP)
      && (asm[i+2].op === "lui" && asm[i+2]?.args[0] === REG.AT)
      && (asm[i+3].op === "j" && asm[i+3]?.args[0] === LABEL_ASSERT)
      && (asm[i+4].opFlags & OP_FLAG_IS_NOP)
      && (asm[i+5].label === asm[i].args[asm[i].args.length - 1])
    ) {
      // point branch directly to assert
      asm[i].op = invertBranchOp(asm[i].op);
      asm[i].args[asm[i].args.length-1] = LABEL_ASSERT;

      const luiOp = asm[i+2];
      asm.splice(i+1, 0, luiOp); // insert the lui instruction in the delay slot
      asm.splice(i+2, 4); // remove the assert instruction and NOP
      i += 2;
    }
  }
}
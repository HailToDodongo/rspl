/**
* @copyright 2024 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {BRANCH_OPS} from "../asmScanDeps.js";

/**
 * If a command only calls another function, we can simplify use that in the command table.
 * The command function can then be ignored.
 *
 * E.g:
 *   T3DCmd_TriSync:
 *     j RDPQ_Triangle_Send_End
 *     nop
 *
 * Here we can remove it completely and put 'RDPQ_Triangle_Send_End' in the cmd. table
 *
 * @param {ASMFunc} asmFunc
 */
export function commandAlias(asmFunc)
{
  const lines = asmFunc.asm;
  if(lines.length < 2 || asmFunc.type !== "command")return;

  const safeBranch = ["beq", "bne", "j", "jr"];
  if(safeBranch.includes(lines[0].op) && lines[1].isNOP) {
    asmFunc.nameOverride = lines[0].args[0];

    if(lines.length == 2) {
      asmFunc.asm = []; // if nothing comes after, we can clear the function
    }
  }
}
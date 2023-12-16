/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../intsructions/asmWriter.js";
import {asmGetReorderRange, BRANCH_OPS, IMMOVABLE_OPS} from "./asmScanDeps.js";
import {dedupeLabels} from "./pattern/dedupeLabels.js";
import {dedupeJumps} from "./pattern/dedupeJumps.js";

/**
 * Optimizes ASM before any dependency analysis.
 * This can be used to strip lines and add pattern-based optimizations.
 * NOTE: At this point delay-slots are always NOPs.
 * @param {ASMFunc} asmFunc
 */
export function asmOptimizePattern(asmFunc)
{
  // strip comments
  asmFunc.asm = asmFunc.asm.filter(line => line.type !== ASM_TYPE.COMMENT);

  dedupeLabels(asmFunc);
  dedupeJumps(asmFunc);
}

/**
 * Optimizes ASM after the initial dependency scan.
 * This will mostly reorder instructions to fill delay-slots,
 * interleave vector instructions, and minimize stalls.
 * @param {ASMFunc} asmFunc
 */
export function asmOptimize(asmFunc)
{
  for(let i=0; i<asmFunc.asm.length; ++i)
  {
    const asm = asmFunc.asm[i];
    if(asm.type !== ASM_TYPE.OP || IMMOVABLE_OPS.includes(asm.op))continue;

    const reorderRange = asmGetReorderRange(asmFunc.asm, i);

    // check if we can move the instruction into a delay slot, this can only happen in the forward-direction.
    const isDelaySlot = asmFunc.asm[reorderRange[1]]?.op === "nop";
    if(isDelaySlot) {
      //console.log("REORDER", asm.op, asm.debug.lineRSPL, reorderRange, isDelaySlot);
      ++asm.debug.reorderCount;
      asmFunc.asm[reorderRange[1]] = asm;
      asmFunc.asm.splice(i, 1);
    }
  }
}
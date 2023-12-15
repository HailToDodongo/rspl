/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../intsructions/asmWriter.js";
import {asmGetReorderRange, BRANCH_OPS, IMMOVABLE_OPS} from "./asmScanDeps.js";

/**
 * Optimizes ASM before any dependency analysis.
 * This can be used to strip lines and add pattern-based optimizations.
 * @param {ASMFunc} asmFunc
 */
export function asmOptimizePattern(asmFunc)
{
  // strip comments
  asmFunc.asm = asmFunc.asm.filter(line => line.type !== ASM_TYPE.COMMENT);

  // de-duplicate labels, first detect duplicates...
  const labelsDelete = [];
  const labelsReplace = {};
  let labels = [];
  for(const asm of asmFunc.asm)
  {
    if(asm.type === ASM_TYPE.LABEL) {
      labels.push(asm.label);
    } else {
      if(labels.length > 1) {
        const newLabel = labels.pop();
        labelsDelete.push(...labels);
        for(const label of labels)labelsReplace[label] = [newLabel];
      }
      labels = [];
    }
  }
  // ...now keep the first one, remove the others and patch the references
  const asmNew = [];
  for(const asm of asmFunc.asm)
  {
    if(labelsDelete.includes(asm.label))continue;
    if(BRANCH_OPS.includes(asm.op)) {
      const label = asm.args[asm.args.length-1];
      if(labelsReplace[label])asm.args[asm.args.length-1] = labelsReplace[label][0];
    }
    asmNew.push(asm);
  }
  asmFunc.asm = asmNew;

  // @TODO: pattern matching, e.g. tail-call optimization
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
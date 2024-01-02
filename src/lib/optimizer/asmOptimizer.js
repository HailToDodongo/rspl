/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../intsructions/asmWriter.js";
import {asmGetReorderRange, BRANCH_OPS, IMMOVABLE_OPS} from "./asmScanDeps.js";
import {dedupeLabels} from "./pattern/dedupeLabels.js";
import {dedupeJumps} from "./pattern/dedupeJumps.js";
import {branchJump} from "./pattern/branchJump.js";
import {evalFunctionCost} from "./eval/evalCost.js";

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
  branchJump(asmFunc);
}

function getRandIndex(minIncl, maxIncl) {
  return Math.floor(Math.random() * (maxIncl - minIncl + 1)) + minIncl;
}

function relocateElement(arr, from, to) {
  arr.splice(to, 0, arr.splice(from, 1)[0]);
  return arr;
}

/**
 * @param {ASMFunc} asmFunc
 */
function optimizeStep(asmFunc)
{
  for(let i=0; i<asmFunc.asm.length; ++i)
  {
    const asm = asmFunc.asm[i];
    if(asm.type !== ASM_TYPE.OP || IMMOVABLE_OPS.includes(asm.op))continue;

    const sourceInDelaySlot = BRANCH_OPS.includes(asmFunc.asm[i-1]?.op);
    if(sourceInDelaySlot)continue; // @TODO

    const reorderRange = asmGetReorderRange(asmFunc.asm, i);
    const targetIdx = getRandIndex(reorderRange[0], reorderRange[1]);

    ++asm.debug.reorderCount;
    const targetEmpty = asmFunc.asm[targetIdx].op === "nop";

    if(targetEmpty) {
      asmFunc.asm[targetIdx] = asm;
      asmFunc.asm.splice(i, 1);
      i = 0;
    } else {
      relocateElement(asmFunc.asm, i, targetIdx);
    }
  }
}

/**
 * Optimizes ASM after the initial dependency scan.
 * This will mostly reorder instructions to fill delay-slots,
 * interleave vector instructions, and minimize stalls.
 * @param {ASMFunc} asmFunc
 */
export function asmOptimize(asmFunc)
{
  const costInit = evalFunctionCost(asmFunc);

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

return; // TEST

  let costBest = evalFunctionCost(asmFunc);
  console.log("costOpt", costInit, costBest);

  let bestASM = [...asmFunc.asm];

  for(let i=0; i<10; ++i) {

    for(let o=0; o<4; ++o) {
      optimizeStep(asmFunc);
    }

    const cost = evalFunctionCost(asmFunc);
    if(cost < costBest) {
      console.log("COST", costInit, cost);
      costBest = cost;
      bestASM = [...asmFunc.asm];
      asmFunc.asm = bestASM;
    }
  }
}
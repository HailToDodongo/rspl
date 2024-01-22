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
import {dedupeImmediate} from "./pattern/dedupeImm.js";
import {mergeSequence} from "./pattern/mergeSequence.js";

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
  dedupeImmediate(asmFunc);
  mergeSequence(asmFunc);
}

let i =0;
function rand() {
  /*i += 34.123;
  return i - Math.floor(i);
  */
  return Math.random();
}

function getRandIndex(minIncl, maxIncl) {
  return Math.floor(rand() * (maxIncl - minIncl + 1)) + minIncl;
}

function getRandShouldReorder() {
  return rand() < 0.4;
}

function relocateElement(arr, from, to) {
  arr.splice(to, 0, arr.splice(from, 1)[0]);
  return arr;
}

/**
 * @param {ASMFunc} asmFunc
 */
function fillDelaySlots(asmFunc)
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

/**
 * @param {ASMFunc} asmFunc
 */
function optimizeStep(asmFunc)
{
  for(let i=0; i<asmFunc.asm.length; ++i)
  {
    const asm = asmFunc.asm[i];
    if(asm.type !== ASM_TYPE.OP || IMMOVABLE_OPS.includes(asm.op))continue;

    const sourceInDelaySlot = asmFunc.asm[i-1]?.opIsBranch;
    if(sourceInDelaySlot)continue; // @TODO

    const reorderRange = asmGetReorderRange(asmFunc.asm, i);

    if(reorderRange[0] === reorderRange[1])continue;
    if(getRandShouldReorder())continue;

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

function reorderRound(asmFunc)
{
  const opCount = Math.floor(rand() * 20);
  for(let o=0; o<opCount; ++o) {
    optimizeStep(asmFunc);
  }
  return {
    cost: evalFunctionCost(asmFunc),
    asm: [...asmFunc.asm]
  }
}

/**
 * Optimizes ASM after the initial dependency scan.
 * This will mostly reorder instructions to fill delay-slots,
 * interleave vector instructions, and minimize stalls.
 * @param {ASMFunc} asmFunc
 * @param {RSPLConfig} config
 */
export function asmOptimize(asmFunc, config)
{
  //fillDelaySlots(asmFunc);
  //return; // TEST

  if(config.reorder)
  {
    let costBest = evalFunctionCost(asmFunc);
    const costInit = costBest;
    console.log("costOpt", costInit);

    let lastRandPick = structuredClone(asmFunc);
    let anyRandPick = structuredClone(asmFunc);

    for(let i=0; i<asmFunc.asm.length*3; ++i)
    //for(let i=0; i<2; ++i)
    {
      if(i % 50 === 0)console.log("Step: ", i, asmFunc.asm.length*5);
      const funcCopy = structuredClone(asmFunc);

      let bestAsm = [...funcCopy.asm];
      for(let s=0; s<10; ++s)
      {
        let refFunc = funcCopy;
        if(s === 0)refFunc = lastRandPick;
        if(s === 1)refFunc = anyRandPick;

        const {cost, asm} = reorderRound(refFunc);
        if(cost < costBest) {
          console.log("COST", costInit, cost);
          costBest = cost;
          bestAsm = asm;
        }

        if(s === 3)lastRandPick = funcCopy;
        if(rand() < 0.2)anyRandPick = funcCopy;
      }

      asmFunc.asm = bestAsm;
    }
  }

  fillDelaySlots(asmFunc);
}
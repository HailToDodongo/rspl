/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE, asmNOP} from "../intsructions/asmWriter.js";
import {amInitDep, asmGetReorderRange} from "./asmScanDeps.js";
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
/*  i += 34.123;
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

/**
 * Relocates an instruction from A to B handling all special cases.
 * @param {ASM[]} arr ASM array
 * @param {number} from index of the instruction to move
 * @param {number} to index of the target instruction
 */
function relocateElement(arr, from, to)
{
  if(from === to || arr[to].opIsBranch)return;
  const targetIsNOP = arr[to]?.isNOP;

  const sourceInDelaySlot = arr[from-1]?.opIsBranch;

  // This covers the 4 possible cases for moving an instruction, based on two factors:
  // A: Is the target a NOP?    -> can be replaced
  // B: Are we in a delay slot? -> needs to generate a NOP as a filler

  // Note: 'to' points to the last safe instruction this means:
  //    if 'to' > 'from' we have place the instruction after 'to'.
  //    if 'to' < 'from' we have to place the instruction before 'to'.

  if(sourceInDelaySlot)
  {
    //console.log("targetOp IN DELAY", from, to, arr[from].op, targetOp);
    if(targetIsNOP) {
      // we can effectively switch the instruction as we need a nop anyway
      arr[to] = arr[from];
    } else {
      // now we need to move to the target, and insert a NOP into our position
      const instr = arr[from];
      arr[from] = asmNOP();
      amInitDep(arr[from]);
      arr.splice(to, 0, instr); // @TODO: test
    }
  } else {
    //console.log("targetOp", from, to, arr[from].op, targetOp);
    if(targetIsNOP) {
      // we can replace the target instruction, and delete our old one
      arr[to] = arr[from];
      arr.splice(from, 1);
    } else {
      // "normal" move, just relocate from A to B
      const instr = arr[from];
      arr.splice(from, 1);
      arr.splice(to, 0, instr);
    }
  }
}

/**
 * @param {ASMFunc} asmFunc
 */
function fillDelaySlots(asmFunc)
{
  for(let i=0; i<asmFunc.asm.length; ++i)
  {
    const asm = asmFunc.asm[i];
    if(asm.type !== ASM_TYPE.OP || asm.opIsImmovable)continue;

    const reorderRange = asmGetReorderRange(asmFunc.asm, i);

    // check if we can move the instruction into a delay slot, this can only happen in the forward-direction.
    const isDelaySlot = asmFunc.asm[reorderRange[1]]?.isNOP;
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
    if(getRandShouldReorder())continue;

    const asm = asmFunc.asm[i];
    const reorderRange = asmGetReorderRange(asmFunc.asm, i);
    if(reorderRange[0] === reorderRange[1])continue;

    let targetIdx = i;
    while(targetIdx === i) {
      targetIdx = getRandIndex(reorderRange[0], reorderRange[1]);
    }

    ++asm.debug.reorderCount;
    relocateElement(asmFunc.asm, i, targetIdx);
  }
}

function reorderRound(asmFunc)
{
  const opCount = Math.floor(rand() * 30) + 2;
  //const opCount = 1;
  for(let o=0; o<opCount; ++o) {
    optimizeStep(asmFunc);
  }
  //console.time("cost");
  const cost = evalFunctionCost(asmFunc);
  //console.timeEnd("cost");
  return {
    cost: cost,
    asm:[...asmFunc.asm]
  }
}

async function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function cloneFunction(func) {
  return structuredClone(func);
  /*const asm = [...func.asm];
  return {...func, asm};*/
}

/**
 * Optimizes ASM after the initial dependency scan.
 * This will mostly reorder instructions to fill delay-slots,
 * interleave vector instructions, and minimize stalls.
 * @param {ASMFunc} asmFunc
 * @param {function} updateCb
 * @param {RSPLConfig} config
 */
export async function asmOptimize(asmFunc, updateCb, config)
{
  if(config.reorder)
  {
    const VARIANT_COUNT = 50;
    const ITERATION_COUNT = asmFunc.asm.length * 1;

    let costBest = evalFunctionCost(asmFunc);
    const costInit = costBest;
    console.log("costOpt", costInit);

    let lastRandPick = cloneFunction(asmFunc);
    let anyRandPick = cloneFunction(asmFunc);
    let mainIterCount = ITERATION_COUNT;
    //let mainIterCount = 15;

    let nothingFoundCount = 0;

    // Main iteration loop
    console.time("StepTime");
    for(let i=0; i<mainIterCount; ++i)
    //for(let i=0; i<2; ++i)
    {
      if(i % 10 === 0) {
        console.timeEnd("StepTime");
        console.log("Step: ", i, mainIterCount);
        console.time("StepTime");
      }
      const funcCopy = cloneFunction(asmFunc);
      let foundNewBest = false;

      // Variants per interation
      let bestAsm = [...funcCopy.asm];
      for(let s=0; s<VARIANT_COUNT; ++s)
      {
        let refFunc = funcCopy;
        if(s < 2)refFunc = lastRandPick;
        else if(s < 4)refFunc = anyRandPick;

        const {cost, asm} = reorderRound(refFunc);
        if(cost < costBest) {
          console.log("====> New Best: ", costInit, cost);
          costBest = cost;
          bestAsm = asm;
          asmFunc.asm = asm;
          foundNewBest = true;
          updateCb(asmFunc);
        }

        //asmFunc.asm = asm; updateCb(asmFunc); await sleep(1200); DEBUG
        await sleep(0);
      }

      if(foundNewBest) {
        mainIterCount += 8;
        nothingFoundCount = 0;
      } else {
        ++nothingFoundCount;
      }

      //console.timeEnd("StepTime");

      if(nothingFoundCount > 200) {
        break;
      }

      if(i % 3 === 0)lastRandPick = funcCopy;
      if(rand() < 0.2)anyRandPick = funcCopy;

      //asmFunc.asm = bestAsm;
      //updateCb(asmFunc);
    }
    console.timeEnd("StepTime");
  } else {
    fillDelaySlots(asmFunc);
  }
}
/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE, asmNOP} from "../intsructions/asmWriter.js";
import {asmInitDep, asmGetReorderRange} from "./asmScanDeps.js";
import {dedupeLabels} from "./pattern/dedupeLabels.js";
import {dedupeJumps} from "./pattern/dedupeJumps.js";
import {branchJump} from "./pattern/branchJump.js";
import {tailCall} from "./pattern/tailCall.js";
import {evalFunctionCost} from "./eval/evalCost.js";
import {dedupeImmediate} from "./pattern/dedupeImm.js";
import {mergeSequence} from "./pattern/mergeSequence.js";
import {removeDeadCode} from "./pattern/removeDeadCode.js";
import {sleep} from "../utils.js";
import {commandAlias} from "./pattern/commandAlias.js";
//import {registerTask, WorkerThreads} from "../workerThreads.js";

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
  tailCall(asmFunc);
  dedupeImmediate(asmFunc);
  mergeSequence(asmFunc);
  removeDeadCode(asmFunc);
  commandAlias(asmFunc);
}

/**
 * ASM Reordering:
 *
 * As a general overview, we want to move instructions around to minimize
 * the predicted cycle count of each function/command.
 * For that, we semi-randomly pick instructions and reorder them.
 *
 * To make this more efficient, the whole process happens in batches with variants.
 * Grouped in: Iterations -> Variants -> (Reorder-)Steps
 *
 * Per iteration, we create multiple variants of the current function, and let them compete.
 * The best one survives and is used as the base for the next iteration.
 * A few random version are saved as a backup to introduce more randomness later on.
 *
 * In each variant, we perform a number of reorder steps, which are semi-randomly picked.
 *
 * Below are settings to fine-tune this process:
 */
const SUB_ITERATION_COUNT    = 15;   // sub-iterations per variant
const PREFER_STALLS_RATE     = 0.33;  // chance to directly fill a stall instead of a random pick

const REORDER_MIN_OPS = 4;  // minimum instructions to reorder per round
const REORDER_MAX_OPS = 15; // maximum instructions to reorder per round


//let i =0;
function rand() {
  //  i += 34.123; return i - Math.floor(i); // DEBUG: fixed randomness
  return Math.random();
}

function getRandIndex(minIncl, maxIncl) {
  return Math.floor(rand() * (maxIncl - minIncl + 1)) + minIncl;
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

  // Note: 'to' points to the last safe instruction this means
  //       that we can always place an instruction before and after the 'to' index.
  //       Depending on 'from' we need to handle this to maximize movement.

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
      asmInitDep(arr[from]);
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
 * Directly fills delay slots with instructions that can be moved.
 * NOTE: with reordering enabled, this function is not needed.
 *       It serves as a backup when reordering is disabled.
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
 * Single attempt to move a random instruction to a random location.
 * @param {ASMFunc} asmFunc
 */
function optimizeStep(asmFunc)
{
  let i = 0;
  let reorderRange = [0,0];

  for(let r=0; r<50 && (reorderRange[0] === reorderRange[1]); ++r) {
    i = getRandIndex(0, asmFunc.asm.length-1);
    reorderRange = asmGetReorderRange(asmFunc.asm, i);
  }
  if(reorderRange[0] === reorderRange[1])return;

  // pick a random target index first, search until we find a new place
  let targetIdx = i;

  // now sometimes we prefer to fill stalls directly, since this prevents any reordering that takes a few moves
  // we cannot do this all the time
  if(rand() < PREFER_STALLS_RATE) {
    let maxStalls = 0;
    for(let j=reorderRange[0]; j<=reorderRange[1]; ++j) {
      let stalls = asmFunc.asm[j].debug.stall || 0;
      if(stalls > maxStalls) {
        maxStalls = stalls;
        targetIdx = j;
      }
    }
  } else {
    while(targetIdx === i) {
      targetIdx = getRandIndex(reorderRange[0], reorderRange[1]);
    }
  }

  ++asmFunc.asm[i].debug.reorderCount;
  relocateElement(asmFunc.asm, i, targetIdx);
}

export function reorderTask(asmFunc) {
  let bestCost = evalFunctionCost(asmFunc);
  let bestFunc = {...asmFunc, asm: [...asmFunc.asm]};
  let refFunc = {...asmFunc, asm: [...asmFunc.asm]};
  for(let run=0; run<asmFunc._iterCount; ++run)
  {
    const result = reorderRound(refFunc);
    if(rand() < 0.2) {
      refFunc.asm = result.asm;
    }

    if(result.cost < bestCost) {
      bestFunc.asm = [...result.asm];
      bestCost = result.cost;
    }
  }
  return {cost: bestCost, asm: bestFunc.asm};
}

/**
 * Round for a single variant, performs a number of reorder steps.
 * @param asmFunc
 * @return {{cost: number, asm: *[]}}
 */
export function reorderRound(asmFunc)
{
  const opCount = Math.floor(rand() * REORDER_MAX_OPS) + REORDER_MIN_OPS;
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
  const funcName = asmFunc.name || "(???)";
  if(config.reorder)
  {
    let costBest = evalFunctionCost(asmFunc);
    asmFunc.cyclesBefore = costBest;
    //const worker = WorkerThreads.getInstance();
    const poolSize = 8;

    const costInit = costBest;
    console.log("costOpt", costInit);

    let totalTime = 0;
    let maxTime = config.optimizeTime || 5_000;
    console.log(`Starting optimization with max. time: ${new Date(maxTime).toISOString().substr(11, 8)}, worker pool size: ${poolSize}`);

    let lastRandPick = cloneFunction(asmFunc);
    let anyRandPick = cloneFunction(asmFunc);
    let historyBest = [];

    // Main iteration loop
    let time = performance.now();
    let i = 0;
    while(totalTime < maxTime)
    {
      if(i !== 0 && (i % 100) === 0) {
        const dur = performance.now() - time;
        totalTime += dur;
        if (totalTime > maxTime) {
          console.log(`[${funcName}] Timeout after ${i} iterations.`);
          break;
        }
        console.log(`[${funcName}] Step: ${i}, Left: ${(maxTime - totalTime).toFixed(4)}ms | Time: ${dur.toFixed(4)}`);
        time = performance.now();
      }
      const funcCopy = cloneFunction(asmFunc);

      // Variants per interation
      let bestAsm = [...funcCopy.asm];
      let results = [];
      //console.time("reorderRound");
      for(let s=0; s<poolSize; ++s)
      {
        let refFunc = funcCopy;
        if(rand() < 0.1)refFunc = lastRandPick;
        if(rand() < 0.2) {
          const randPick = Math.floor(rand() * historyBest.length);
          if(historyBest[randPick])refFunc = {...refFunc, asm: [...historyBest[randPick]]};
        }

        refFunc._iterCount = Math.floor(i / 600) + SUB_ITERATION_COUNT;
        //results.push(worker.runTask("reorder", refFunc));
        results.push(reorderRound(refFunc));
      }

      results = await Promise.all(results);

      for(let s=0; s<results.length; ++s)
      {
        const {cost, asm} = results[s];
        const isBetter = cost < costBest;
        const isSame = cost === costBest;
        const canUseTheSame = s < (results.length / 2);

        if(isBetter || (canUseTheSame && isSame)) {
          costBest = cost;
          bestAsm = asm;
          asmFunc.asm = asm;
          asmFunc.cyclesAfter = cost;

          if(isBetter) {
            historyBest.push(asm);
            updateCb(asmFunc);
            console.log(`[${funcName}] \x1B[32m**** New Best for '${funcName}': ${costInit} -> ${cost} ****\x1B[0m`);
          }
        }
      }

      if(i % 3 === 0)lastRandPick = funcCopy;
      if(rand() < 0.5)anyRandPick = funcCopy;
      ++i;
      await sleep();
    }

  } else {
    fillDelaySlots(asmFunc);
  }
}
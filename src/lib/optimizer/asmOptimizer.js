/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE, asmNOP} from "../intsructions/asmWriter.js";
import {
  asmInitDep,
  asmGetReorderIndices,
  OP_FLAG_IS_BRANCH,
  OP_FLAG_IS_IMMOVABLE,
  OP_FLAG_IS_VECTOR, OP_FLAG_IS_NOP
} from "./asmScanDeps.js";
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
import {assertCompare} from "./pattern/assertCompare.js";
//import {registerTask, WorkerThreads} from "../workerThreads.js";

/**
 * Optimizes ASM before any dependency analysis.
 * This can be used to strip lines and add pattern-based optimizations.
 * NOTE: At this point delay-slots are always NOPs.
 * @param {ASMFunc} asmFunc
 */
export function asmOptimizePattern(asmFunc)
{
  dedupeLabels(asmFunc);
  dedupeJumps(asmFunc);
  branchJump(asmFunc);
  tailCall(asmFunc);
  dedupeImmediate(asmFunc);
  mergeSequence(asmFunc);
  assertCompare(asmFunc);
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
const POOL_SIZE            = 8;   // sub-iterations per variant
const PREFER_STALLS_RATE   = 0.20;  // chance to directly fill a stall instead of a random pick
const PREFER_PAIR_RATE     = 0.80;  // chance to directly fill a stall instead of a random pick

const MAX_STEPS_NO_CHANGE  = 6000; // steps with no better results until we generate new variants
const SEARCH_VARIANT_SEARCH = 6; // how many variants to check when searching a worse solution
const SEARCH_BACK_STEPS_FACTOR = 10; // when searching for new variants, how many steps to go back mult. by the consecutive amount of failed attempts
const SEARCH_FWD_STEPS_FACTOR = 5;

const REORDER_MIN_OPS = 3;  // minimum instructions to reorder per round
const REORDER_MAX_OPS = 15; // maximum instructions to reorder per round

let seed = Math.floor(Math.random() * 0xFFFFFF);
//seed = 0xDEADBEEF;
function rand() {
  seed = (seed * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
  return (seed >>> 16) / 0x10000;
}

function getRandIndex(maxExcl) {
  seed = (seed * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
  return (seed >>> 16) % maxExcl;
}

/**
 * Relocates an instruction from A to B handling all special cases.
 * @param {ASM[]} arr ASM array
 * @param {number} from index of the instruction to move
 * @param {number} to index of the target instruction
 */
function relocateElement(arr, from, to)
{
  if(from === to || arr[to].opFlags & OP_FLAG_IS_BRANCH)return;
  const targetIsNOP = arr[to]?.opFlags & OP_FLAG_IS_NOP;

  const sourceInDelaySlot = arr[from-1]?.opFlags & OP_FLAG_IS_BRANCH;

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
    if(asm.type !== ASM_TYPE.OP || (asm.opFlags & OP_FLAG_IS_IMMOVABLE))continue;

    const reorderRange = asmGetReorderIndices(asmFunc.asm, i);

    // check if we can move the instruction into a delay slot, this can only happen in the forward-direction.
    let delaySlotIdx = -1;
    for(let idx of reorderRange) {
      if(asmFunc.asm[idx].opFlags & OP_FLAG_IS_NOP) {
        delaySlotIdx = idx;
        break;
      }
    }

    if(delaySlotIdx >= 0) {
      //console.log("REORDER", asm.op, asm.debug.lineRSPL, reorderRange, isDelaySlot);
      ++asm.debug.reorderCount;
      asmFunc.asm[delaySlotIdx] = asm;
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
  let reorderIndices = [];

  for(let r=0; r<50 && (reorderIndices.length <= 1); ++r) {
    i = getRandIndex(asmFunc.asm.length);
    reorderIndices = asmGetReorderIndices(asmFunc.asm, i);
  }
  if(reorderIndices.length <= 1)return;

  // pick a random target index first, search until we find a new place
  let targetIdx = i;

  // now sometimes we prefer to fill stalls directly, since this prevents any reordering that takes a few moves
  // we cannot do this all the time
  let foundIndex = false;
  if(rand() < PREFER_PAIR_RATE) {
    for(let j of reorderIndices) {
      if((asmFunc.asm[j].opFlags & OP_FLAG_IS_VECTOR) !== (asmFunc.asm[i].opFlags & OP_FLAG_IS_VECTOR)
      ) {
        let paired = asmFunc.asm[j].debug.paired;
        if(!paired) {
          targetIdx = j;
          foundIndex = true;
        }
      }
    }
    if(!foundIndex)return;
  }

  if(!foundIndex && rand() < PREFER_STALLS_RATE) {
    let maxStalls = 0;
    for(let j of reorderIndices) {
      let stalls = asmFunc.asm[j].debug.stall || 0;
      if(stalls > maxStalls) {
        maxStalls = stalls;
        targetIdx = j;
        foundIndex = true;
      }
    }
  }

  if(!foundIndex) {
    while(targetIdx === i) {
      targetIdx = reorderIndices[getRandIndex(reorderIndices.length)];
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
 * @param {ASMFunc} asmFunc
 * @param {number} steps
 * @return ASMFunc
 */
function generateWorseFunction(asmFunc, steps) {
  let maxCost = 0;
  let newWorst = asmFunc;
  for(let i=0; i<steps; ++i) {
    const f = cloneFunction(asmFunc);
    reorderRound(f);
    reorderRound(f);
    const cost = evalFunctionCost(f);
    if(cost > maxCost) {
      newWorst = f;
      maxCost = cost;
    }
  }

  return [newWorst, maxCost];
}

/**
 * Round for a single variant, performs a number of reorder steps.
 * @param asmFunc
 * @return {{cost: number, asm: *[]}}
 */
export function reorderRound(asmFunc)
{
  const opCount = getRandIndex(REORDER_MAX_OPS) + REORDER_MIN_OPS;
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

/**
 *
 * @param {ASMFunc} func
 * @return {ASMFunc}
 */
function cloneFunction(func) {
  //return structuredClone(func);
  const asm = [...func.asm];
  return {...func, asm};
}

/**
 * Optimizes ASM after the initial dependency scan.
 * This will mostly reorder instructions to fill delay-slots,
 * interleave vector instructions, and minimize stalls.
 * @param {ASMFunc} asmFunc
 * @param {function|undefined} updateCb
 * @param {RSPLConfig} config
 */
export async function asmOptimize(asmFunc, updateCb, config)
{
  const funcName = asmFunc.name || "(???)";
  if(!config.reorder) {
    fillDelaySlots(asmFunc);
    return;
  }

  const doSleep = typeof window !== "undefined";
  let costBest = evalFunctionCost(asmFunc);
  asmFunc.cyclesBefore = costBest;
  //const worker = WorkerThreads.getInstance();
  const poolSize = POOL_SIZE;

  const costInit = costBest;
  let totalTime = 0;
  let maxTime = config.optimizeTime || 10_000;
  console.log(`Starting optimization with max. time: ${new Date(maxTime).toISOString().substr(11, 8)}, worker pool size: ${poolSize}`);

  let lastRandPick = cloneFunction(asmFunc);

  // Main iteration loop
  let time = performance.now();
  let timeEnd = time + maxTime;
  let i = 0;
  let stepsSinceLastOpt = 0;
  let consecutiveSame = 0;
  while(totalTime < maxTime)
  {
    if(i !== 0 && (i % 1000) === 0)
    {
      const dur = performance.now() - time;
      totalTime += dur;
      console.log(`[${funcName}] Step: ${i}, Left: ${(maxTime - totalTime).toFixed(4)}ms | Time: ${dur.toFixed(4)}s`);
      time = performance.now();
    }

    if (performance.now() > timeEnd) {
      console.log(`[${funcName}] Timeout after ${i} iterations.`);
      break;
    }

    const funcCopy = cloneFunction(asmFunc);

    // Variants per interation
    const results = [];

    if(stepsSinceLastOpt > MAX_STEPS_NO_CHANGE) {
      ++consecutiveSame;
      const stepsBack = consecutiveSame * SEARCH_BACK_STEPS_FACTOR;
      const stepsFwd = consecutiveSame * SEARCH_FWD_STEPS_FACTOR;
      console.log(`[${funcName}] ${stepsSinceLastOpt} steps since last improvement, generate new versions (${stepsBack} steps backward)`);


      // try to get "unstuck" from a potential local-minimum
      // this works by intentionally generating worse versions of the function (aka walking uphill)
      // which my discover a new path to a smaller minimum
      for(let s=0; s<SEARCH_VARIANT_SEARCH; ++s) {
        let [worseCopy, maxCost] = generateWorseFunction(funcCopy, stepsBack);
        for(let t=0; t<stepsFwd; ++t) {
          const worseCopyTry = cloneFunction(worseCopy);
          reorderRound(worseCopyTry);
          const cost = evalFunctionCost(worseCopyTry);
          if(cost < maxCost) {
            // we found a better version, use that instead
            worseCopy.asm = worseCopyTry.asm;
            maxCost = cost;
          }
        }

        results.push(reorderRound(worseCopy));
      }
      for(let s=SEARCH_VARIANT_SEARCH; s<poolSize; ++s) {
        results.push(reorderRound(rand() < 0.1 ? lastRandPick : funcCopy));
      }

      stepsSinceLastOpt = 0;
    } else {
      //console.time("reorderRound");
      for(let s=0; s<poolSize; ++s) {
        results.push(reorderRound(rand() < 0.1 ? lastRandPick : funcCopy));
      }
    }

    for(let s=0; s<results.length; ++s)
    {
      const {cost, asm} = results[s];
      const isBetter = cost < costBest;
      const isSame = cost === costBest;
      const canUseTheSame = s < (results.length / 4);

      if(isBetter || (canUseTheSame && isSame)) {
        costBest = cost;
        asmFunc.asm = asm;
        asmFunc.cyclesAfter = cost;

        if(isBetter) {
          if(updateCb)updateCb(asmFunc);
          console.log(`[${funcName}] \x1B[32m**** New Best for '${funcName}': ${costInit} -> ${cost} ****\x1B[0m`);
          stepsSinceLastOpt = 0;
          consecutiveSame = 0;
        }
      }
    }

    if(i % 3 === 0)lastRandPick = funcCopy;
    ++i;
    ++stepsSinceLastOpt;
    if(doSleep)await sleep();
  }
}
/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";

// Reference/Docs: https://n64brew.dev/wiki/Reality_Signal_Processor/CPU_Pipeline

const BRANCH_STEP_NONE = 0;
const BRANCH_STEP_BRANCH = 1;
const BRANCH_STEP_DELAY = 2;

/**
 * Evaluates the cost of a given function.
 * This performs cycle counting and calculates an overall score intended to compare optimizations.
 * Since it evaluates all possible branches, it will not reflect the real-world cycle count.
 * @param {ASMFunc} asmFunc
 * @return {number} score
 */
export function evalFunctionCost(asmFunc)
{
  /** @type {OptInstruction[]} instructions that are currently executing (incl. latencies) */
  let cycle = 0;
  let regCycleMap = {}; // register to stall counter
  let regLastWriteMask = 0n; // bitfield of registers that where written to in the last cycle

  let lastIsVector = false;
  let inDualIssue = true;
  let branchStep = 0;
  let lastLoadPosMask = 0; // if exactly 2, causes a stall. (counts up)

  // advances a single cycle, updating the active instructions in the process
  const tick = (advanceDeps = true) => {
    if(advanceDeps) {
      for(const reg in regCycleMap) {
        --regCycleMap[reg];
      }
    }
    ++cycle;
    lastLoadPosMask <<= 1;
    lastLoadPosMask &= 0xFF; // JS sucks at letting bits fall off the end
  }

  for(const asm of asmFunc.asm)
  {
    if(asm.type !== ASM_TYPE.OP)continue;
    asm.debug.stall = 0;

    if(asm.opIsMemStallLoad)lastLoadPosMask |= 1;

    // check if one of our source or destination reg as written to in the last instruction
    const maskSrcTarget = asm.depsStallSourceMask | asm.depsStallTargetMask;
    const hadWriteLastInstr = (regLastWriteMask & maskSrcTarget) !== 0n;

    //console.log("Tick: ", cycle, asm.op, [...asm.depsStallTarget], {ld: lastLoadPos, b: branchStep});

    // check if we can in theory dual-issue
    //const couldDualIssue = lastIsVector !== asm.opIsVector && branchStep !== 2 && !hadWriteLastInstr;
    const couldDualIssue = lastIsVector !== asm.opIsVector
        && branchStep !== BRANCH_STEP_DELAY
        && !hadWriteLastInstr;

    lastIsVector = asm.opIsVector;
    regLastWriteMask = asm.depsStallTargetMask;

    // branch "state-machine", keep track where we currently are
    const isBranch = asm.opIsBranch;
    if(branchStep === BRANCH_STEP_DELAY)branchStep = BRANCH_STEP_NONE;
    else if(branchStep === BRANCH_STEP_BRANCH)branchStep = BRANCH_STEP_DELAY;
    else if(branchStep === BRANCH_STEP_NONE && isBranch)branchStep = BRANCH_STEP_BRANCH;

    let hasRegDep = false;
    for(const regSrc of asm.depsStallSource) {
      if(regCycleMap[regSrc] > 0) {
        hasRegDep = true;
        break;
      }
    }

    // cause special stall if we have a load 2 instr. after a load
    let isLoadDelay = false;
    if(lastLoadPosMask & 0b100 && asm.opIsMemStallStore) {
      //console.log("LOAD STALL", asm.op, hasRegDep);
      ++asm.debug.stall;
      tick(hasRegDep);
      isLoadDelay = true;
    }

    // check for actual dual-issue (vector/scalar right after each other)
    inDualIssue = (!inDualIssue && couldDualIssue);
    if(!inDualIssue)tick(); // only tick if we can not dual-issue

    // check if all our source registers are ready, otherwise wait
    for(const regSrc of asm.depsStallSource) {
      while(regCycleMap[regSrc] > 0) {
        ++asm.debug.stall;
        tick();
      }
    }

    asm.debug.cycle = cycle;

    const latency = asm.stallLatency;
    for(const regDst of asm.depsStallTarget) {
      regCycleMap[regDst] = latency;
    }
  }
  return cycle;
}
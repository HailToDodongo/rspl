/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE, asmNOP} from "../../intsructions/asmWriter.js";
import {
  OP_FLAG_CTC2_CFC2,
  OP_FLAG_IS_BRANCH, OP_FLAG_IS_LIKELY,
  OP_FLAG_IS_MEM_STALL_LOAD,
  OP_FLAG_IS_MEM_STALL_STORE,
  OP_FLAG_IS_VECTOR, OP_FLAG_LIKELY_BRANCH
} from "../asmScanDeps.js";

// Reference/Docs: https://n64brew.dev/wiki/Reality_Signal_Processor/CPU_Pipeline

const BRANCH_STEP_NONE = 0;
const BRANCH_STEP_BRANCH = 2;
const BRANCH_STEP_DELAY = 1;

/**
 * Evaluates the cost of a given function.
 * This performs cycle counting and calculates an overall score intended to compare optimizations.
 * Since it evaluates all possible branches, it will not reflect the real-world cycle count.
 * @param {ASMFunc} asmFunc
 * @return {number} score
 */
export function evalFunctionCost(asmFunc)
{
  let execCount = 0;

  let branchStep = 0;
  let lastLoadPosMask = 0;
  let regCycleMap = []; // register to stall counter

  for(let i=0; i<64; ++i) {
    regCycleMap[i] = 0;
  }

  let didJump = false;
  let cycle = 0;
  let pc = 0;
  /** @type {ASM[]} */
  let ops = asmFunc.asm.filter(asm => asm.type === ASM_TYPE.OP);

  const ticks = (count) => {
    for(let i=0; i<64; ++i)regCycleMap[i] -= count;
    lastLoadPosMask = lastLoadPosMask >>> count;
    cycle += count;
  };

  while(pc < ops.length)
  {
    let lastCycle;
    do {
      lastCycle = cycle;
      for(let i=0; i<execCount; ++i) {
        const execOp = ops[pc + i];
        execOp.debug.paired = execCount === 2;

        // check if one of our source or destination reg as written to in the last instruction
        for(const regSrc of execOp.depsStallSourceIdx) {
          if(regCycleMap[regSrc] > 0)ticks(regCycleMap[regSrc]);
        }
        // if a store happens exactly two cycles after a load, stall it
        if(lastLoadPosMask & 0b001 && (execOp.opFlags & OP_FLAG_IS_MEM_STALL_STORE)) {
          ++execOp.debug.stall;
          ticks(1);
        }
      }
    } while (lastCycle !== cycle); // run until all stalls are resolved

    // now execute
    for(let i=0; i<execCount; ++i) {
      const execOp = ops[pc + i];
      if(execOp.opFlags & OP_FLAG_IS_MEM_STALL_LOAD)lastLoadPosMask |= 0b100;
      didJump ||= (execOp.opFlags & OP_FLAG_LIKELY_BRANCH);

      // update where in a branch we are (branch, delay, none)
      branchStep >>>= 1;
      if(!branchStep && (execOp.opFlags & OP_FLAG_IS_BRANCH) !== 0)branchStep = BRANCH_STEP_BRANCH;

      // if branch is taken, an unavoidable bubble is inserted
      if(didJump && branchStep === BRANCH_STEP_DELAY) {
        ticks(1); didJump = false;
      }

      // now "execute" by marking the target regs with stalls
      execOp.debug.cycle = cycle;
      for(const regDst of execOp.depsStallTargetIdx) {
        regCycleMap[regDst] = execOp.stallLatency;
      }
    }

    // fetch next instructions to execute
    pc += execCount;
    const op = ops[pc];
    const opNext = ops[pc+1];

    // now check if we can dual-issue, this is always checked from the perspective of the current instruction
    // seeing if the next one can be used
    let canDualIssue = opNext // needs something after to dual issue
      && (op.opFlags & OP_FLAG_IS_VECTOR) !== (opNext.opFlags & OP_FLAG_IS_VECTOR) // can only do SU/VU or VU/SU
      && !(branchStep === BRANCH_STEP_BRANCH) // cannot dual issue if in delay slot
      && !(op.opFlags & OP_FLAG_IS_BRANCH) // cannot dual issue with the delay slot
      // if a vector reg is written to, and the next instr. reads from it, do not dual issue
      && (op.depsStallTargetMask0 & opNext.depsStallSourceMask0) === 0
      && (op.depsStallTargetMask1 & opNext.depsStallSourceMask1) === 0
      && (op.depsStallTargetMask0 & opNext.depsStallTargetMask0) === 0
      && (op.depsStallTargetMask1 & opNext.depsStallTargetMask1) === 0;

    if(canDualIssue) {
      let ctrlRWMask = opNext.depsSourceMask | opNext.depsTargetMask; // incl. control regs here as an exception
      if((opNext.opFlags & OP_FLAG_CTC2_CFC2) && (op.depsTargetMask & ctrlRWMask) !== 0n) {
        canDualIssue = false;
      }
    }

    execCount = canDualIssue ? 2 : 1;
    ticks(1);
  }
  return cycle;
}
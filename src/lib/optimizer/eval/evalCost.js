/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE, asmNOP} from "../../intsructions/asmWriter.js";

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
  /** @type {Array<ASM>} */
  let execOps = [];

  let branchStep = 0;
  let lastLoadPosMask = 0;
  let regCycleMap = {}; // register to stall counter

  let didJump = false;
  let cycle = 0;
  let pc = 0;
  /** @type {ASM[]} */
  let ops = asmFunc.asm.filter(asm => asm.type === ASM_TYPE.OP);

  const tick = () => {
    for(const reg in regCycleMap)--regCycleMap[reg];
    lastLoadPosMask = (lastLoadPosMask << 1) & 0xFF;
    ++cycle;
  };

  while(pc === 0 || execOps.length)
  {
    let hadStall; // wait out any stalls

    if(execOps[0]) {
      execOps[0].debug.paired = false;
      if(execOps.length === 2) {
        execOps[0].debug.paired = true;
        execOps[1].debug.paired = true;
      }
    }

    do {
      hadStall = false;
      for(let execOp of execOps) {
        // check if one of our source or destination reg as written to in the last instruction
        for(const regSrc of execOp.depsStallSource) {
          while(regCycleMap[regSrc] > 0) {
            ++execOp.debug.stall;
            hadStall = true;
            tick();
          }
        }
        // if a store happens exactly two cycles after a load, stall it
        if(lastLoadPosMask & 0b100 && execOp.opIsMemStallStore) {
          ++execOp.debug.stall;
          hadStall = true;
          tick();
        }
      }
    } while (hadStall);

    // now execute
    for(let execOp of execOps)
    {
      if(execOp.opIsMemStallLoad)lastLoadPosMask |= 1;
      if(execOp.opIsBranch && execOp.isLikely) {
        didJump = true;
      }

      // update where in a branch we are (branch, delay, none)
      if(branchStep === BRANCH_STEP_DELAY)branchStep = BRANCH_STEP_NONE;
        else if(branchStep === BRANCH_STEP_BRANCH)branchStep = BRANCH_STEP_DELAY;
      if(branchStep === BRANCH_STEP_NONE && execOp.opIsBranch)branchStep = BRANCH_STEP_BRANCH;

      // if branch is taken, an unavoidable bubble is inserted
      if(didJump && branchStep === BRANCH_STEP_DELAY) {
        tick(); didJump = false;
      }

      // now "execute" by marking the target regs with stalls
      execOp.debug.cycle = cycle;
      for(const regDst of execOp.depsStallTarget) {
        regCycleMap[regDst] = execOp.stallLatency;
      }
    }

    // fetch next instructions to execute
    if(pc >= ops.length)break;
    const opPrev = ops[pc-1];
    const op = ops[pc];
    const opNext = ops[pc+1];

    // now check if we can dual-issue, this is always checked from the perspective of the current instruction
    // seeing if the next one can be used
    let canDualIssue = opNext // needs something after to dual issue
      && op.opIsVector !== opNext.opIsVector // can only do SU/VU or VU/SU
      && !(opPrev ? opPrev.opIsBranch : false) // cannot dual issue if in delay slot
      && !op.opIsBranch; // cannot dual issue with the delay slot

    // if a vector reg is written to, and the next instr. reads from it, do not dual issue
    if(canDualIssue) {
      if((op.depsStallTargetMask & opNext.depsStallSourceMask) !== 0n ||
         (op.depsStallTargetMask & opNext.depsStallTargetMask) !== 0n) {
        canDualIssue = false;
      }

      let ctrlRWMask = opNext.depsSourceMask | opNext.depsTargetMask; // incl. control regs here as an exception
      if((opNext.op === "cfc2" || opNext.op === "ctc2") && (op.depsTargetMask & ctrlRWMask) !== 0n) {
        canDualIssue = false;
      }
    }

    execOps = canDualIssue ? [op, opNext] : [op];
    pc += canDualIssue ? 2 : 1;
    tick();
  }
  return cycle;
}
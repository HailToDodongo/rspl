/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE, asmNOP} from "../../intsructions/asmWriter.js";

// Reference/Docs: https://n64brew.dev/wiki/Reality_Signal_Processor/CPU_Pipeline

const BRANCH_STEP_NONE = 0;
const BRANCH_STEP_BRANCH = 1;
const BRANCH_STEP_DELAY = 2;

export function evalFunctionCostNew(asmFunc)
{
  /** @type {Array<ASM>} */
  let execOps = [];

  let branchStep = 0;
  let hadWriteLastInstr = false; // @TODO
  let lastLoadPosMask = 0;
  let regCycleMap = {}; // register to stall counter

  let didJump = false;
  let cycle = 0;
  let pc = 0;
  /** @type {ASM[]} */
  let ops = asmFunc.asm.filter(asm => asm.type === ASM_TYPE.OP);

  const tick = (advanceDeps = true) => {
    if(advanceDeps) {
      for(const reg in regCycleMap) {
        --regCycleMap[reg];
      }
    }
    ++cycle;
    lastLoadPosMask <<= 1;
    lastLoadPosMask &= 0xFF; // JS sucks at letting bits fall off the end
  };

  while(pc === 0 || execOps.length)
  {
    // wait out any stalls
    for(;;)
    {
      let hadStall = false;
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
      if(!hadStall)break;
    }

    let lastWasDelay = false;
    // now execute
    for(let execOp of execOps) {
      const latency = execOp.stallLatency;

      if(execOp.opIsMemStallLoad)lastLoadPosMask |= 1;

      const isBranch = execOps[0].opIsBranch || execOps[1]?.opIsBranch;
      if(isBranch && (execOps[0]?.isLikely || execOps[1]?.isLikely)) {
        didJump = true;
      }

      // update where in a branch we are (branch, delay, none)
      if(branchStep === BRANCH_STEP_DELAY)branchStep = BRANCH_STEP_NONE;
        else if(branchStep === BRANCH_STEP_BRANCH)branchStep = BRANCH_STEP_DELAY;
      if(branchStep === BRANCH_STEP_NONE && isBranch)branchStep = BRANCH_STEP_BRANCH;

      if(didJump && branchStep === BRANCH_STEP_DELAY) {
        tick(); didJump = false;
        lastWasDelay = true;
      }

      execOp.debug.cycle = cycle;
      for(const regDst of execOp.depsStallTarget) {
        regCycleMap[regDst] = latency;
      }
    }

    // fetch next instructions to execute
    if(pc >= ops.length)break;
    const opPrev = ops[pc-1];
    const op = ops[pc];
    const nextOp = ops[pc+1];

    let canDualIssue = nextOp
      && op.opIsVector !== nextOp.opIsVector
      && !(opPrev ? opPrev.opIsBranch : false) // cannot dual issue if in delay slot
      && !hadWriteLastInstr;

    // cannot dual issue with the delay slot
    if(op.opIsBranch)canDualIssue = false;

    // if a vector reg is written to, and the next instr. reads from it, do not dual issue
    if(nextOp) {
      if((op.depsStallTargetMask & nextOp.depsStallSourceMask) !== 0n ||
         (op.depsStallTargetMask & nextOp.depsStallTargetMask) !== 0n) {
        canDualIssue = false;
      }

      let ctrlRWMask = op.depsSourceMask | op.depsTargetMask; // incl. control regs here as an exception
      /*if((op.op === "cfc2" || op.op === "ctc2") && (nextOp.depsTargetMask & ctrlRWMask) !== 0n) {
        canDualIssue = false;
      }*/
      ctrlRWMask = nextOp.depsSourceMask | nextOp.depsTargetMask; // incl. control regs here as an exception
      if((nextOp.op === "cfc2" || nextOp.op === "ctc2") && (op.depsTargetMask & ctrlRWMask) !== 0n) {
        canDualIssue = false;
      }
    }

    execOps = [op];
    if(canDualIssue)execOps.push(nextOp);
    pc += canDualIssue ? 2 : 1;
    tick();
  }

  return 0;
}

/**
 * Evaluates the cost of a given function.
 * This performs cycle counting and calculates an overall score intended to compare optimizations.
 * Since it evaluates all possible branches, it will not reflect the real-world cycle count.
 * @param {ASMFunc} asmFunc
 * @return {number} score
 */
export function evalFunctionCost(asmFunc)
{
  return evalFunctionCostNew(asmFunc);
  /** @type {OptInstruction[]} instructions that are currently executing (incl. latencies) */
  let cycle = 0;
  let regCycleMap = {}; // register to stall counter
  let regLastWriteMask = 0n; // bitfield of registers that where written to in the last cycle

  let lastIsVector = false;
  let inDualIssue = true;
  let branchStep = 0;
  let lastLoadPosMask = 0; // if exactly 2, causes a stall. (counts up)
  let didJump = true;

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

  let log = "";

  let lastCtrlRWMask = 0n;
  for(const asm of asmFunc.asm)
  {
    if(asm.type !== ASM_TYPE.OP)continue;
    asm.debug.stall = 0;

    if(asm.opIsMemStallLoad)lastLoadPosMask |= 1;

    // check if one of our source or destination reg as written to in the last instruction
    const maskSrcTarget = asm.depsStallSourceMask | asm.depsStallTargetMask;
    const hadWriteLastInstr = (regLastWriteMask & maskSrcTarget) !== 0n;

    //console.log("Tick: ", cycle, asm.op, [...asm.depsStallTarget], {ld: lastLoadPos, b: branchStep});

    // special check if a previous instruction wrote to a control register
    let isC2Blocked = (asm.op === "cfc2" || asm.op === "ctc2") && (asm.depsSourceMask & lastCtrlRWMask) !== 0n;

    regLastWriteMask = asm.depsStallTargetMask;
    lastCtrlRWMask = asm.depsSourceMask | asm.depsTargetMask; // incl. control regs here as an exception

    const oneAfterDelay = branchStep === BRANCH_STEP_DELAY;

    // branch "state-machine", keep track where we currently are
    const isBranch = asm.opIsBranch;
         if(branchStep === BRANCH_STEP_DELAY)branchStep = BRANCH_STEP_NONE;
    else if(branchStep === BRANCH_STEP_BRANCH)branchStep = BRANCH_STEP_DELAY;

    if(branchStep === BRANCH_STEP_NONE && isBranch)branchStep = BRANCH_STEP_BRANCH;

    if(isBranch) {
      didJump = asm.isLikely;
    }

    // check if we can in theory dual-issue
    //const couldDualIssue = lastIsVector !== asm.opIsVector && branchStep !== 2 && !hadWriteLastInstr;
    const couldDualIssue = lastIsVector !== asm.opIsVector
        && branchStep !== BRANCH_STEP_DELAY
        && !hadWriteLastInstr
        && !oneAfterDelay
        && !isC2Blocked;

    lastIsVector = asm.opIsVector;

    let hasRegDep = false;
    for(const regSrc of asm.depsStallSource) {
      if(regCycleMap[regSrc] > 0) {
        hasRegDep = true;
        break;
      }
    }

    // cause special stall if we have a load 2 instr. after a load
    while(lastLoadPosMask & 0b100 && asm.opIsMemStallStore) {
      ++asm.debug.stall;
      tick();
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

    //  branch bubble (only if taken)
    if(branchStep === BRANCH_STEP_DELAY) {
      if(didJump)tick();
      didJump = false;
    }
    asm.debug.cycle = cycle;

    const latency = asm.stallLatency;
    for(const regDst of asm.depsStallTarget) {
      regCycleMap[regDst] = latency;
    }
  }
  return cycle;
}
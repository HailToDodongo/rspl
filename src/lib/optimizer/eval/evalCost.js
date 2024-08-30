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

  //let log = "";

  let lastCtrlRWMask = 0n;
  for(const asm of asmFunc.asm)
  {
    if(asm.type !== ASM_TYPE.OP)continue;
    asm.debug.stall = 0;

    if(asm.opIsMemStallLoad)lastLoadPosMask |= 1;

    //if(global.LOG_ON)log += `Tick[${cycle}]: ${asm.op}\t| lat: ${asm.stallLatency}`;

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

    if(branchStep === BRANCH_STEP_DELAY)tick();
    asm.debug.cycle = cycle;
    //if(global.LOG_ON)log += `\t| cycle: ${cycle}\n`;

    const latency = asm.stallLatency;
    for(const regDst of asm.depsStallTarget) {
      regCycleMap[regDst] = latency;
    }
  }
  //if(log)console.log(log);
  return cycle;
}
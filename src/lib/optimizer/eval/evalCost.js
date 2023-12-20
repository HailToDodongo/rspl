/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {LOAD_OPS_SCALAR, LOAD_OPS_VECTOR} from "../asmScanDeps.js";

// Reference/Docs: https://n64brew.dev/wiki/Reality_Signal_Processor/CPU_Pipeline

/** @param {string} op */
function isVectorOp(op) {
  return op.startsWith("v");
}

/** @param {string} op */
function getBaseLatency(op) {
  if(isVectorOp(op) || op === "mtc2")return 4;
  if(LOAD_OPS_VECTOR.includes(op))return 4;
  if(LOAD_OPS_SCALAR.includes(op))return 3;
  if(["mfc0", "mfc2", "cfc2"].includes(op))return 3;
  return 1;
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
  /** @type {OptInstruction[]} instructions that are currently executing (incl. latencies) */
  let activeInstr = []; // @TODO: use register lookup instead?
  let cycle = 1;

  // advances a single cycle, updating the active instructions in the process
  const tick = () => {
    for(const a of activeInstr)a.latency--;
    activeInstr = activeInstr.filter(a => a.latency > 0);
    ++cycle;
  }
  let lastIsVector = !isVectorOp(asmFunc.asm[0].op); // force a missmatch on the first instruction

  for(const asm of asmFunc.asm)
  {
    if(asm.type !== ASM_TYPE.OP)continue;

    // check for dual-issue (vector/scalar right after each other)
    const isVector = isVectorOp(asm.op);
    let isDualIssue = lastIsVector !== isVector;
    lastIsVector = isDualIssue ? !isVector : isVector;

    // check if all our source registers are ready, otherwise wait
    for(const regSrc of asm.depsStallSource) {
      while(activeInstr.some(prev => prev.asm.depsStallTarget.includes(regSrc))) {
        //console.log(asm.op, reg, structuredClone(activeInstr));
        tick();
        isDualIssue = false;
      }
    }

    if(!isDualIssue)tick();

    asm.debug.cycle = cycle;

    const latency = getBaseLatency(asm.op);
    activeInstr.push({latency, asm});
  }
  return cycle;
}
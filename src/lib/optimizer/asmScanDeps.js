/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {isVecReg, REG} from "../syntax/registers.js";
import {SWIZZLE_LANE_MAP} from "../syntax/swizzle.js";
import {ASM_TYPE, asmLabel} from "../intsructions/asmWriter.js";
import {difference, hasIntersection, intersect} from "../utils.js";

// ops that save data to RAM, and only read from regs
export const STORE_OPS = [
  "sw", "sh", "sb",
  "sbv", "ssv", "slv", "sdv", "sqv",
  "spv", "suv"
];

export const LOAD_OPS_SCALAR = ["lw", "lh", "lb"];
export const LOAD_OPS_VECTOR = ["lbv", "lsv", "llv", "ldv", "lqv", "lpv", "luv"];

// ops that load from RAM, r/w register access
export const LOAD_OPS = [...LOAD_OPS_SCALAR, ...LOAD_OPS_VECTOR];

export const BRANCH_OPS = ["beq", "bne", "j", "jr", "jal"];

// ops that don't write to any register
export const READ_ONLY_OPS = [...BRANCH_OPS, ...STORE_OPS];
// ops not allowed to be moved, other instructions can still be moved around them
export const IMMOVABLE_OPS = [...BRANCH_OPS, "nop"];

// registers which can be considered constant, can be ignored for dependencies
export const CONST_REGS = [REG.ZERO, REG.VZERO, REG.VSHIFT, REG.VSHIFT8];

// regs used by instructions, but not listed as arguments
const HIDDEN_REGS_READ = {
  "vlt" : [       "$vco"       ],
  "veq" : [       "$vco"       ],
  "vne" : [       "$vco"       ],
  "vge" : [       "$vco"       ],
  "vmrg": ["$vcc"              ],
  "vcl" : [       "$vco",      ],
  "vmacf":[              "$acc"],
  "vmacu":[              "$acc"],
  "vmudn":[              "$acc"],
  "vmadn":[              "$acc"],
  "vmudl":[              "$acc"],
  "vmadl":[              "$acc"],
  "vmudm":[              "$acc"],
  "vmadm":[              "$acc"],
  "vmudh":[              "$acc"],
  "vmadh":[              "$acc"],
  "vrndp":[              "$acc"],
  "vrndn":[              "$acc"],
  "vmacq":[              "$acc"],
  "vsar" :[              "$acc"],
  "vrcph":[    "$DIV_OUT"      ],
  "vrcpl":[    "$DIV_IN"       ],
  "vrsql":[    "$DIV_IN"       ],
  "vadd": [      "$vco",       ],
  "vsub": [      "$vco",       ],
};

// regs written by instructions, but not listed as arguments
const HIDDEN_REGS_WRITE = {
  "vlt" : ["$vcc", "$vco", "$acc"],
  "veq" : ["$vcc", "$vco", "$acc"],
  "vne" : ["$vcc", "$vco", "$acc"],
  "vge" : ["$vcc", "$vco", "$acc"],
  "vch" : ["$vcc", "$vco", "$acc"],
  "vcr" : ["$vcc", "$vco", "$acc"],
  "vcl" : ["$vcc", "$vco", "$acc"],
  "vmrg": [        "$vco", "$acc"],
  "vmov": [                "$acc"],
  "vrcp": [                "$acc", "$DIV_OUT"],
  "vrcph":[                "$acc", "$DIV_IN" ],
  "vrsq": [                        "$DIV_OUT"],
  "vrsqh":[                "$acc", "$DIV_IN" ],
  "vrcpl":[                "$acc", "$DIV_OUT", "$DIV_IN"],
  "vrsql":[                "$acc", "$DIV_OUT", "$DIV_IN"],
  "vadd": [        "$vco", "$acc"],
  "vsub": [        "$vco", "$acc"],
  "vaddc":[        "$vco", "$acc"],
  "vsubc":[        "$vco", "$acc"],
  "vand": [                "$acc"],
  "vnand":[                "$acc"],
  "vor":  [                "$acc"],
  "vnor": [                "$acc"],
  "vxor": [                "$acc"],
  "vnxor":[                "$acc"],
  "vmulf":[                "$acc"],
  "vmulu":[                "$acc"],
  "vmacf":[                "$acc"],
  "vmacu":[                "$acc"],
  "vmudn":[                "$acc"],
  "vmadn":[                "$acc"],
  "vmudl":[                "$acc"],
  "vmadl":[                "$acc"],
  "vmudm":[                "$acc"],
  "vmadm":[                "$acc"],
  "vmudh":[                "$acc"],
  "vmadh":[                "$acc"],
  "vrndp":[                "$acc"],
  "vrndn":[                "$acc"],
  "vmulq":[                "$acc"],
  "vmacq":[                "$acc"],
};

const STALL_IGNORE_REGS = ["$vcc", "$vco", "$acc", "$DIV_OUT", "$DIV_IN"];

/**
 * Expands vector registers into separate lanes.
 * Does nothing for scalar registers.
 * @param {string} regName
 * @returns {string[]}
 */
function expandRegister(regName) {
  let [reg, lane] = regName.split(".");
  if(isVecReg(reg)) {
    lane = lane || "v";
    return SWIZZLE_LANE_MAP[lane].map(i => reg+"_"+i);
  }
  return [regName];
}

/**
 * @param {ASM} line
 */
export function getTargetRegs(line) {
  if(READ_ONLY_OPS.includes(line.op)) {
    return [];
  }
  const targetReg = ["mtc2"].includes(line.op) ? line.args[1] : line.args[0];
  return [targetReg, ...HIDDEN_REGS_WRITE[line.op] || []]
    .filter(Boolean)
}

/** @param {ASM} line */
function getSourceRegs(line)
{
  if(["jr", "mtc2"].includes(line.op)) {
    return [line.args[0]];
  }
  if(["beq", "bne"].includes(line.op)) {
    return [line.args[0], line.args[1]]; // 3rd arg is the label
  }
  if(line.opIsStore) {
    return line.args;
  }
  if(line.op === "j") {
    return [];
  }
  const res = line.args.slice(1);
  res.push(...(HIDDEN_REGS_READ[line.op] || []));
  return res;
}

/**
 * @param {ASM} line
 * @returns {string[]}
 */
function getSourceRegsFiltered(line)
{
  return getSourceRegs(line)
    .filter(reg => typeof reg === "string")
    .map(reg => {
      // extract register from brackets (e.g. writes with offset)
      const brIdx = reg.indexOf("(");
      if(brIdx >= 0)return reg.substring(brIdx+1, reg.length-1);
      return reg;
    })
    .filter(arg => (arg+"").startsWith("$"));
    //.filter(reg => !CONST_REGS.includes(reg));
}

/**
 * Inits data necessary for further dependency analysis.
 * This mostly sets up the source/target registers for each instruction.
 * @param {ASMFunc} asmFunc
 */
export function asmInitDeps(asmFunc)
{
  for(const asm of asmFunc.asm) {
    if(asm.type !== ASM_TYPE.OP) {
      asm.depsSource = [];
      asm.depsTarget = [];
      asm.depsStallSource = [];
      asm.depsStallTarget = [];
      continue;
    }

    asm.depsStallSource = [...new Set(getSourceRegsFiltered(asm))];
    asm.depsStallTarget = [...new Set(getTargetRegs(asm))];

    asm.depsSource = asm.depsStallSource.flatMap(expandRegister);
    asm.depsTarget = asm.depsStallTarget.flatMap(expandRegister);

    asm.depsStallSource = asm.depsStallSource
      .map(reg => reg.split(".")[0])
      .filter(reg => !STALL_IGNORE_REGS.includes(reg));

    asm.depsStallTarget = asm.depsStallTarget
      .map(reg => reg.split(".")[0])
      .filter(reg => !STALL_IGNORE_REGS.includes(reg));
  }
}

/**
 * @param {ASM} asm
 * @param {ASM} asmPrev
 * @return {boolean} true if there is a dependency
 */
function checkAsmBackwardDep(asm, asmPrev) {
  // stop at any label
  if(asm.type !== ASM_TYPE.OP || asmPrev.type !== ASM_TYPE.OP) {
    return true;
  }

  // Don't reorder writes to RAM, this is an oversimplification.
  // For a more accurate check, the RAM location/size would need to be checked (if possible).
  const isLoad = asm.opIsLoad;
  const isStore = !isLoad && asm.opIsStore;
  if(isLoad || isStore) { // memory access can be ignored if it's not a load or store
    const isLoadPrev = asmPrev.opIsLoad;
    const isStorePrev = !isLoadPrev && asmPrev.opIsStore;

    // load cannot be put before a previous store (previous load is ok)
    if(isLoad && isStorePrev) {
      return true;
    }
    // store cannot be put before a previous load or store
    //if(isStore && (isLoadPrev || isStorePrev)) {
    if(isStore && isLoadPrev) {
      return true;
    }
  }

  // check if any of our source registers is a destination of a previous instruction, and the reserve.
  // (otherwise our read would see a different value if reordered)
  // (otherwise out write could change what the previous instruction(s) reads)
  return (
     hasIntersection(asmPrev.depsTarget, asm.depsSource) // prev. writes to our source
  || hasIntersection(asmPrev.depsSource, asm.depsTarget) // prev. reads from our target
 );
}

/**
 * Returns the min/max index of where an instruction can be reordered to.
 * @param {ASM[]} asmList
 * @param {number} i
 * @param {string[]} returnRegs
 * @return {[number, number]} min/max index
 */
export function asmGetReorderRange(asmList, i, returnRegs = [])
{
  const asm = asmList[i];
  const isInDelaySlot = asmList[i-1]?.opIsBranch;

  if(asm.type !== ASM_TYPE.OP || IMMOVABLE_OPS.includes(asm.op) || isInDelaySlot) {
    return [i, i];
  }

  // Scan ahead...
  let lastWrite = {};
  const lastRead = {};
  let pos = asmList.length;

  let f = i + 1;
  for(; f < asmList.length; ++f) {
    const asmNext = asmList[f];
    for(const reg of asmNext.depsSource)lastRead[reg] = f;

    // stop at a branch with an already filled delay-slot,
    // or once we are past the delay-slot of a branch if it is empty.
    const isEmptyBranch = asmList[f+1]?.op !== "nop" && asmNext.opIsBranch;
    const isPastBranch = asmList[f-2]?.opIsBranch;

    if(isEmptyBranch || isPastBranch || checkAsmBackwardDep(asmNext, asm)) {
      pos = f;
      break;
    }

    // Remember the last write that occurs for each register, this is used to fall back if we stop at a read.
    for(const reg of asmNext.depsTarget) {
      lastWrite[reg] = f;
    }
  }

  // even though we (may have) found a dependency, we still need to know if something tries to
  // read from one of our sources. Not doing so would prevent us from reordering the last write of a chain.
  for(; f < asmList.length; ++f) {
    const asmNext = asmList[f];
    for(const reg of asmNext.depsSource)lastRead[reg] = f;
  }

  // check if there was an instruction in between which wrote to one of our target registers.
  // if true, fall-back to that position (otherwise register would contain wrong value)
  for(const reg of asm.depsTarget) {
    if(lastWrite[reg] && (lastRead[reg] || returnRegs.includes(reg))) {
      pos = Math.min(lastWrite[reg], pos);
    }
  }

  const minMax = [0, pos-1];

  // collect all registers that where not overwritten by any instruction after us.
  // these need to be checked for writes in the backwards-scan.
  const writeCheckRegs = difference(asm.depsTarget, Object.keys(lastWrite));

  // go backwards through all instructions before...
  for(let b=i-1; b >= 0; --b)
  {
    const asmPrev = asmList[b];

    // stop at the delay-slot of a branch, we cannot fill it backwards
    const asmPrevPrev = asmList[b-1];
    const isBranch = asmPrevPrev && asmPrevPrev.opIsBranch;

    if(isBranch || checkAsmBackwardDep(asm, asmPrev)
      || hasIntersection(asmPrev.depsTarget, writeCheckRegs)
    ) {
      minMax[0] = b+1;
      break;
    }
  }

  return minMax;
}
/**
 * @param {ASMFunc} asmFunc
 */
export function asmScanDeps(asmFunc)
{
  for(let i = 0; i < asmFunc.asm.length; ++i) {
    const minMax = asmGetReorderRange(asmFunc.asm, i);
    asmFunc.asm[i].debug.reorderLineMin = asmFunc.asm[minMax[0]]?.debug.lineASM;
    asmFunc.asm[i].debug.reorderLineMax = asmFunc.asm[minMax[1]]?.debug.lineASM;
  }
}
/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {isVecReg, REG} from "../syntax/registers.js";
import {SWIZZLE_LANE_MAP} from "../syntax/swizzle.js";
import {ASM_TYPE} from "../intsructions/asmWriter.js";

// ops that save data to RAM, and only read from regs
const STORE_OPS = [
  "sw", "sh", "sb",
  "sbv", "ssv", "slv", "sdv", "sqv",
  "spv", "suv"
];

// ops that load from RAM, r/w register access
const LOAD_OPS = [
  "lw", "lh", "lb",
  "lbv", "lsv", "llv", "ldv", "lqv",
  "lpv", "luv"
];

const BRANCH_OPS = ["beq", "bne", "j", "jr", "jal"];

// ops that don't write to any register
const READ_ONLY_OPS = [...BRANCH_OPS, ...STORE_OPS];
// ops not allowed to be moved, other instructions can still be moved around them
const IMMOVABLE_OPS = [...BRANCH_OPS, "nop"];

// registers which can be considered constant, can be ignored for dependencies
const CONST_REGS = [REG.ZERO, REG.VZERO, REG.VSHIFT, REG.VSHIFT8];

// regs used by instructions, but not listed as arguments
const HIDDEN_REGS_READ = {
  "vmrg": ["VCC"              ],
  "vcl" : [       "VC0",      ],
  "vmacf":[              "ACC"],
  "vmacu":[              "ACC"],
  "vmudn":[              "ACC"],
  "vmadn":[              "ACC"],
  "vmudl":[              "ACC"],
  "vmadl":[              "ACC"],
  "vmudm":[              "ACC"],
  "vmadm":[              "ACC"],
  "vmudh":[              "ACC"],
  "vmadh":[              "ACC"],
  "vrndp":[              "ACC"],
  "vrndn":[              "ACC"],
  "vmacq":[              "ACC"],
  "vsar" :[              "ACC"],
  "vrcph":[     "DIV_OUT"     ],
  "vrcpl":[     "DIV_IN"      ],
  "vrsql":[     "DIV_IN"      ],
};

// regs written by instructions, but not listed as arguments
const HIDDEN_REGS_WRITE = {
  "vlt" : ["VCC", "VCO", "ACC"],
  "veq" : ["VCC", "VCO", "ACC"],
  "vne" : ["VCC", "VCO", "ACC"],
  "vge" : ["VCC", "VCO", "ACC"],
  "vch" : ["VCC", "VCO", "ACC"],
  "vcr" : ["VCC", "VCO", "ACC"],
  "vcl" : ["VCC",        "ACC"],
  "vmrg": [              "ACC"],
  "vmov": [              "ACC"],
  "vrcp": [              "ACC", "DIV_OUT"],
  "vrcph":[              "ACC", "DIV_IN" ],
  "vrsq": [                     "DIV_OUT"],
  "vrsqh":[              "ACC", "DIV_IN" ],
  "vrcpl":[              "ACC", "DIV_OUT", "DIV_IN"],
  "vrsql":[              "ACC", "DIV_OUT", "DIV_IN"],
  "vadd": [       "VCO", "ACC"],
  "vsub": [       "VCO", "ACC"],
  "vaddc":[       "VCO", "ACC"],
  "vsubc":[       "VCO", "ACC"],
  "vand": [              "ACC"],
  "vnand":[              "ACC"],
  "vor":  [              "ACC"],
  "vnor": [              "ACC"],
  "vxor": [              "ACC"],
  "vnxor":[              "ACC"],
  "vmulf":[              "ACC"],
  "vmulu":[              "ACC"],
  "vmacf":[              "ACC"],
  "vmacu":[              "ACC"],
  "vmudn":[              "ACC"],
  "vmadn":[              "ACC"],
  "vmudl":[              "ACC"],
  "vmadl":[              "ACC"],
  "vmudm":[              "ACC"],
  "vmadm":[              "ACC"],
  "vmudh":[              "ACC"],
  "vmadh":[              "ACC"],
  "vrndp":[              "ACC"],
  "vrndn":[              "ACC"],
  "vmulq":[              "ACC"],
  "vmacq":[              "ACC"],
};

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
function getTargetRegs(line) {
  if(READ_ONLY_OPS.includes(line.op)) {
    return [];
  }
  const targetReg = ["mtc2"].includes(line.op) ? line.args[1] : line.args[0];
  return [targetReg, ...HIDDEN_REGS_WRITE[line.op] || []]
    .filter(Boolean)
    .flatMap(expandRegister);
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
  if(STORE_OPS.includes(line.op)) {
    return line.args;
  }
  if(line.op === "j") {
    return [];
  }
  const res = line.args.slice(1);
  res.push(...HIDDEN_REGS_READ[line.op] || []);
  return res;
}

/** @param {ASM} line */
function getSourceRegsFiltered(line)
{
  let regs = getSourceRegs(line)
    .filter(reg => typeof reg === "string")
    .map(reg => {
      // extract register from brackets (e.g. writes with offset)
      const brIdx = reg.indexOf("(");
      if(brIdx >= 0)return reg.substring(brIdx+1, reg.length-1);
      return reg;
    })
    .filter(arg => (arg+"").startsWith("$"))
    .filter(reg => !CONST_REGS.includes(reg))
    .flatMap(expandRegister);
  return [...new Set(regs)]; // make unique
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
      continue;
    }

    asm.depsSource = getSourceRegsFiltered(asm);
    asm.depsTarget = getTargetRegs(asm);
  }
}

/**
 * @param {ASM} asm
 * @param {ASM} asmPrev
 * @return {boolean} true if there is a dependency
 */
function checkAsmBackwardDep(asm, asmPrev) {
  // Don't reorder writes to RAM, this is an oversimplification.
  // For a more accurate check, the RAM location/size would need to be checked (if possible).
  const isLoad = LOAD_OPS.includes(asm.op);
  const isStore = !isLoad && STORE_OPS.includes(asm.op);
  if(isLoad || isStore) { // memory access can be ignored if it's not a load or store
    const isLoadPrev = LOAD_OPS.includes(asmPrev.op);
    const isStorePrev = !isLoadPrev && STORE_OPS.includes(asmPrev.op);

    // load cannot be put before a previous store (previous load is ok)
    if(isLoad && isStorePrev) {
      return true;
    }
    // store cannot be put before a previous load or store
    if(isStore && (isLoadPrev || isStorePrev)) {
      return true;
    }
  }

  // register based dependency check
  return !!(
     asmPrev.depsTarget.find(x => asm.depsSource.includes(x)) // prev. writes to our source
  || asmPrev.depsSource.find(x => asm.depsTarget.includes(x)) // prev. reads from our target
  || asmPrev.depsTarget.find(x => asm.depsTarget.includes(x))
 );
}

/**
 * Returns the min/max index of where an instruction can be reordered to.
 * @param {ASM[]} asmList
 * @param {number} i
 * @return {[number, number]} min/max index
 */
export function asmGetReorderRange(asmList, i)
{
  const asm = asmList[i];

  if(asm.type !== ASM_TYPE.OP || IMMOVABLE_OPS.includes(asm.op)) {
    return [i, i];
  }

  const minMax = [-1, asmList.length];

  // there is chance out target register(s) where also written to by an instruction in between.
  // This is ok if nothing reads from it, but if something does, we have to stop before the last write.
  // @TODO: pre-scan

  // go backwards through all instructions before...
  for(let b=i-1; b >= 0; --b)
  {
    const asmPrev = asmList[b];

    // stop at the delay-slot of a branch, we cannot fill it backwards
    const asmPrevPrev = asmList[b-1];
    const isBranch = asmPrevPrev && BRANCH_OPS.includes(asmPrevPrev.op);

    // check if any of our source registers is a destination of a previous instruction, and the reserve.
    // (otherwise our read would see a different value if reordered)
    // (otherwise out write could change what the previous instruction(s) reads)
    if(isBranch || asmPrev.type !== ASM_TYPE.OP || checkAsmBackwardDep(asm, asmPrev, false)) {
      minMax[0] = b;
      break;
    }
  }

  // Now scan ahead...
  for(let f=i+1; f < asmList.length; ++f) {
    const asmNext = asmList[f];

    // stop at a branch, this time we have to include the delay-slot
    const asmLast = asmList[f-2];
    const isBranch = asmLast && BRANCH_OPS.includes(asmLast.op);

    // Same logic as in the backwards-scan, but with reversed instructions.
    if(isBranch || asmNext.type !== ASM_TYPE.OP || checkAsmBackwardDep(asmNext, asm, true)) {
      minMax[1] = f;
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
    asmFunc.asm[i].debug.reorderLineMin = asmFunc.asm[minMax[0]]?.debug;
    asmFunc.asm[i].debug.reorderLineMax = asmFunc.asm[minMax[1]]?.debug;
  }
}
/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {isVecReg, REG} from "../syntax/registers.js";
import {SWIZZLE_LANE_MAP} from "../syntax/swizzle.js";

// ops that save data to RAM, and only read from regs
const SAVE_OPS = [
  "sw", "sh", "sb",
  "sbv", "ssv", "slv", "sdv", "sqv",
  "spv", "suv"
];

// ops that don't write to any register
const READ_ONLY_OPS = ["beq", "bne", "j", "jr", ...SAVE_OPS];
// registers which can be considered constant, can be ignored for dependencies
const CONST_REGS = [REG.ZERO, REG.VZERO, REG.VSHIFT, REG.VSHIFT8];

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
// @TODO: handle save instructions
  const targetReg = ["mtc2"].includes(line.op) ? line.args[1] : line.args[0];
  // @TODO: handle hidden registers (e.g. VCC)
  return [targetReg]
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
  if(SAVE_OPS.includes(line.op)) {
    return line.args;
  }
  if(line.op === "j") {
    return [];
  }
  return line.args.slice(1);
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
    .flatMap(expandRegister); // remove lane (@TODO: handle this?)
  return [...new Set(regs)]; // make unique
}

/**
 * @param {ASMFunc} asmFunc
 */
export function asmToTree(asmFunc)
{
  // get all source and target registers per assembly line
  /** [{}] */
  const regDeps = [];
  for(const line of asmFunc.asm) {
    regDeps.push({
      source: getSourceRegsFiltered(line),
      target: getTargetRegs(line)
    });
  }

  // for each instruction, map the source register to the last instruction that modified it.
  // for target registers, map it to al previous instructions that read from it, until an earlier write is found.
  for(let i = 0; i < asmFunc.asm.length; ++i) {
    const deps = regDeps[i];
    const line = asmFunc.asm[i];

    let sourceRegs = [...deps.source];
    let targetRegs = [...deps.target];
    let targetRegsWrite = []; // filled if a reg in targetRegs is used by another write

    // go backwards through all instructions before...
    for(let b=i-1; b >= 0; --b) {
      const lastDeps = regDeps[b];
      // check if any of our source registers is a destination of a previous instruction
      // (otherwise our read would see a different value if reordered)
      const matchingRegs = lastDeps.target.filter(x => sourceRegs.includes(x));
      if(matchingRegs.length)
      {
        //deps.ref = asmFunc.asm[b];
        line.debug.lineDepsASM.push(asmFunc.asm[b].debug);

        // we found a dependency, all matching registers can be removed for the next iteration
        sourceRegs = sourceRegs.filter(x => !matchingRegs.includes(x));
      }

      if(targetRegs.length) {
        // check if any of our target registers is a source of a previous instruction
        // (otherwise out write could change what the previous instruction(s) reads)
        const matchingTargetRegs = lastDeps.source.filter(x => targetRegs.includes(x));
        if(matchingTargetRegs.length)
        {
          line.debug.lineDepsASM.push(asmFunc.asm[b].debug);
          //targetRegs = targetRegs.filter(x => !matchingTargetRegs.includes(x));
        }
      }

      if(sourceRegs.length === 0 && targetRegs.length === 0)break;
    }
    console.log(line.op, deps.source, deps.target, deps.ref);
  }
}
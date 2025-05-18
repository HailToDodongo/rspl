/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {isVecReg, REG, REG_COP0} from "../syntax/registers.js";
import {SWIZZLE_LANE_MAP} from "../syntax/swizzle.js";
import {ASM_TYPE} from "../intsructions/asmWriter.js";
import state from "../state.js";

// ops that save data to RAM, and only read from regs
export const STORE_OPS = [
  "sw", "sh", "sb",
  "sbv", "ssv", "slv", "sdv", "sqv",
  "spv", "suv", "shv", "sfv", "stv", "swv", "srv"
];

export const LOAD_OPS_SCALAR = ["lw", "lh", "lhu", "lb", "lbu"];
export const LOAD_OPS_VECTOR = ["lbv", "lsv", "llv", "ldv", "lqv", "lpv", "luv", "ltv", "lrv"];

// ops that load from RAM, r/w register access
export const LOAD_OPS = [...LOAD_OPS_SCALAR, ...LOAD_OPS_VECTOR];

export const BRANCH_OPS = ["beq", "bne", "bgezal", "bltzal", "bgez", "bltz", "j", "jr", "jal"];

// ops that don't write to any register
export const READ_ONLY_OPS = [...BRANCH_OPS, ...STORE_OPS, "mtc0"];
// ops not allowed to be moved, other instructions can still be moved around them
export const IMMOVABLE_OPS = [...BRANCH_OPS, "nop"];

export const MEM_STALL_LOAD_OPS  = [...LOAD_OPS, "mfc0", "mtc0", "mfc2", "mtc2", "cfc2", "ctc2", "catch"];
export const MEM_STALL_STORE_OPS = [...STORE_OPS, "mfc0", "mtc0", "mfc2", "mtc2", "cfc2", "ctc2", "catch"];

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
  "vrsq": [                "$acc", "$DIV_OUT"],
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

const REG_INDEX_MAP = {
  "$v00": 0,    "$v00_0": 0, "$v00_1": 1, "$v00_2": 2, "$v00_3": 3, "$v00_4": 4, "$v00_5": 5, "$v00_6": 6, "$v00_7": 7,
  "$v01": 8,    "$v01_0": 8, "$v01_1": 9, "$v01_2":10, "$v01_3":11, "$v01_4":12, "$v01_5":13, "$v01_6":14, "$v01_7":15,
  "$v02":16,    "$v02_0":16, "$v02_1":17, "$v02_2":18, "$v02_3":19, "$v02_4":20, "$v02_5":21, "$v02_6":22, "$v02_7":23,
  "$v03":24,    "$v03_0":24, "$v03_1":25, "$v03_2":26, "$v03_3":27, "$v03_4":28, "$v03_5":29, "$v03_6":30, "$v03_7":31,
  "$v04":32,    "$v04_0":32, "$v04_1":33, "$v04_2":34, "$v04_3":35, "$v04_4":36, "$v04_5":37, "$v04_6":38, "$v04_7":39,
  "$v05":40,    "$v05_0":40, "$v05_1":41, "$v05_2":42, "$v05_3":43, "$v05_4":44, "$v05_5":45, "$v05_6":46, "$v05_7":47,
  "$v06":48,    "$v06_0":48, "$v06_1":49, "$v06_2":50, "$v06_3":51, "$v06_4":52, "$v06_5":53, "$v06_6":54, "$v06_7":55,
  "$v07":56,    "$v07_0":56, "$v07_1":57, "$v07_2":58, "$v07_3":59, "$v07_4":60, "$v07_5":61, "$v07_6":62, "$v07_7":63,
  "$v08":64,    "$v08_0":64, "$v08_1":65, "$v08_2":66, "$v08_3":67, "$v08_4":68, "$v08_5":69, "$v08_6":70, "$v08_7":71,
  "$v09":72,    "$v09_0":72, "$v09_1":73, "$v09_2":74, "$v09_3":75, "$v09_4":76, "$v09_5":77, "$v09_6":78, "$v09_7":79,
  "$v10":80,    "$v10_0":80, "$v10_1":81, "$v10_2":82, "$v10_3":83, "$v10_4":84, "$v10_5":85, "$v10_6":86, "$v10_7":87,
  "$v11":88,    "$v11_0":88, "$v11_1":89, "$v11_2":90, "$v11_3":91, "$v11_4":92, "$v11_5":93, "$v11_6":94, "$v11_7":95,
  "$v12":96,    "$v12_0":96, "$v12_1":97, "$v12_2":98, "$v12_3":99, "$v12_4":100,"$v12_5":101,"$v12_6":102,"$v12_7":103,
  "$v13":104,    "$v13_0":104,"$v13_1":105,"$v13_2":106,"$v13_3":107,"$v13_4":108,"$v13_5":109,"$v13_6":110,"$v13_7":111,
  "$v14":112,    "$v14_0":112,"$v14_1":113,"$v14_2":114,"$v14_3":115,"$v14_4":116,"$v14_5":117,"$v14_6":118,"$v14_7":119,
  "$v15":120,    "$v15_0":120,"$v15_1":121,"$v15_2":122,"$v15_3":123,"$v15_4":124,"$v15_5":125,"$v15_6":126,"$v15_7":127,
  "$v16":128,    "$v16_0":128,"$v16_1":129,"$v16_2":130,"$v16_3":131,"$v16_4":132,"$v16_5":133,"$v16_6":134,"$v16_7":135,
  "$v17":136,    "$v17_0":136,"$v17_1":137,"$v17_2":138,"$v17_3":139,"$v17_4":140,"$v17_5":141,"$v17_6":142,"$v17_7":143,
  "$v18":144,    "$v18_0":144,"$v18_1":145,"$v18_2":146,"$v18_3":147,"$v18_4":148,"$v18_5":149,"$v18_6":150,"$v18_7":151,
  "$v19":152,    "$v19_0":152,"$v19_1":153,"$v19_2":154,"$v19_3":155,"$v19_4":156,"$v19_5":157,"$v19_6":158,"$v19_7":159,
  "$v20":160,    "$v20_0":160,"$v20_1":161,"$v20_2":162,"$v20_3":163,"$v20_4":164,"$v20_5":165,"$v20_6":166,"$v20_7":167,
  "$v21":168,    "$v21_0":168,"$v21_1":169,"$v21_2":170,"$v21_3":171,"$v21_4":172,"$v21_5":173,"$v21_6":174,"$v21_7":175,
  "$v22":176,    "$v22_0":176,"$v22_1":177,"$v22_2":178,"$v22_3":179,"$v22_4":180,"$v22_5":181,"$v22_6":182,"$v22_7":183,
  "$v23":184,    "$v23_0":184,"$v23_1":185,"$v23_2":186,"$v23_3":187,"$v23_4":188,"$v23_5":189,"$v23_6":190,"$v23_7":191,
  "$v24":192,    "$v24_0":192,"$v24_1":193,"$v24_2":194,"$v24_3":195,"$v24_4":196,"$v24_5":197,"$v24_6":198,"$v24_7":199,
  "$v25":200,    "$v25_0":200,"$v25_1":201,"$v25_2":202,"$v25_3":203,"$v25_4":204,"$v25_5":205,"$v25_6":206,"$v25_7":207,
  "$v26":208,    "$v26_0":208,"$v26_1":209,"$v26_2":210,"$v26_3":211,"$v26_4":212,"$v26_5":213,"$v26_6":214,"$v26_7":215,
  "$v27":216,    "$v27_0":216,"$v27_1":217,"$v27_2":218,"$v27_3":219,"$v27_4":220,"$v27_5":221,"$v27_6":222,"$v27_7":223,
  "$v28":224,    "$v28_0":224,"$v28_1":225,"$v28_2":226,"$v28_3":227,"$v28_4":228,"$v28_5":229,"$v28_6":230,"$v28_7":231,
  "$v29":232,    "$v29_0":232,"$v29_1":233,"$v29_2":234,"$v29_3":235,"$v29_4":236,"$v29_5":237,"$v29_6":238,"$v29_7":239,
  "$v30":240,    "$v30_0":240,"$v30_1":241,"$v30_2":242,"$v30_3":243,"$v30_4":244,"$v30_5":245,"$v30_6":246,"$v30_7":247,
  "$v31":248,    "$v31_0":248,"$v31_1":249,"$v31_2":250,"$v31_3":251,"$v31_4":252,"$v31_5":253,"$v31_6":254,"$v31_7":255,

  "$zero": 256, "$at": 257, "$v0": 258, "$v1": 259, "$a0": 260, "$a1": 261, "$a2": 262, "$a3": 263,
  "$t0": 264, "$t1": 265, "$t2": 266, "$t3": 267, "$t4": 268, "$t5": 269, "$t6": 270, "$t7": 271,
  "$s0": 272, "$s1": 273, "$s2": 274, "$s3": 275, "$s4": 276, "$s5": 277, "$s6": 278, "$s7": 279,
  "$t8": 280, "$t9": 281,
  "$k0": 282, "$k1": 283, "$gp": 284, "$sp": 285, "$fp": 286, "$ra": 287,

  "$vco": 288, "$vcc": 289, "$acc": 290, "$DIV_OUT": 291, "$DIV_IN": 292,

  SIZE: 293
};

const LTV_REG_MAP = {
  "$v00": ["$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07"],
  "$v08": ["$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15"],
  "$v16": ["$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23"],
  "$v24": ["$v24", "$v25", "$v26", "$v27", "$v28", "$v29", "$v30", "$v31"],
};

const REG_MASK_MAP = {SIZE: REG_INDEX_MAP.SIZE};
let REG_MASK_ALL = 0n;

for(let reg of Object.keys(REG_INDEX_MAP)) {
  REG_MASK_MAP[reg] = BigInt(1) << BigInt(REG_INDEX_MAP[reg]);
  REG_MASK_ALL |= REG_MASK_MAP[reg];
}

/**
 *
 * @param {string[]} regs
 * @return BigInt
 */
function getRegisterMask(regs)
{
  let regMask = BigInt(0);
  for(const r of regs) {
    const regIdx = REG_MASK_MAP[r];
    if(regIdx === undefined)throw new Error("Unknown register: "+r);
    regMask |= BigInt(regIdx);
  }
  return regMask;
}

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

  if(line.opIsLoad) {
    // transpose, access 8 registers and lanes in a diagonal pattern
    if(line.op === 'ltv')
    {
      const mainReg = line.args[0] || '$v00';
      const row = parseInt(line.args[1]) / 2;

      let regs = [...LTV_REG_MAP[mainReg]];
      if(!regs)state.throwError(`Invalid base register ${mainReg} for ltv!`);

      for(let i=0; i < 8; ++i) {
        regs[i] += ".e" + ((8 + i - row) % 8);
      }
      return regs;
    }
  }

  const targetReg = ["mtc2"].includes(line.op) ? line.args[1] : line.args[0];
  return [targetReg, ...HIDDEN_REGS_WRITE[line.op] || []]
    .filter(Boolean)
}

/** @param {ASM} line */
export function getSourceRegs(line)
{
  if(["jr", "mtc2", "mtc0", "ctc2"].includes(line.op)) {
    return [line.args[0]];
  }
  if(line.opIsBranch && line.op.startsWith("b")) {
    // last arg is label, take all before that
    return line.args.slice(0, -1);
  }
  if(line.opIsStore)
  {
    // transpose, access 8 registers and lanes in a diagonal pattern
    if(line.op === 'stv')
    {
      const mainReg = line.args[0] || '$v00';
      const row = parseInt(line.args[1]) / 2;

      let regs = [...LTV_REG_MAP[mainReg]];
      if(!regs)state.throwError(`Invalid base register ${mainReg} for stv!`);

      for(let i=0; i < 8; ++i) {
        regs[i] += ".e" + ((8 + i - row) % 8);
      }
      regs.push(line.args[line.args.length-1]);
      return regs;
    }

    return line.args;
  }
  if(["j", "jal"].includes(line.op)) {
    return [line.args[0]];
  }
  const res = line.args.slice(1);
  res.push(...(HIDDEN_REGS_READ[line.op] || []));
  return res;
}

/**
 * @param {ASM} line
 * @returns {string[]}
 */
export function getSourceRegsFiltered(line)
{
  return getSourceRegs(line)
    .filter(reg => typeof reg === "string")
    .map(reg => {
      // extract register from brackets (e.g. writes with offset)
      const brIdx = reg.lastIndexOf("(");
      if(brIdx >= 0)return reg.substring(brIdx+1, reg.length-1);
      return reg;
    })
    .filter(arg => (arg+"").startsWith("$"));
}

/**
 * @param {ASM} asm
 */
export function asmInitDep(asm)
{
  if(asm.type !== ASM_TYPE.OP || asm.isNOP) {
    asm.depsSource = [];
    asm.depsTarget = [];
    asm.depsStallSource = [];
    asm.depsStallTarget = [];
    asm.depsSourceMask = 0n;
    asm.depsTargetMask = 0n;
    asm.depsBlockSourceMask = REG_MASK_ALL;
    asm.depsBlockTargetMask = REG_MASK_ALL;
    asm.depsStallSourceMask = 0n;
    asm.depsStallTargetMask = 0n;
    asm.barrierMask = 0;
    asm.depsArgMask = 0n;
    return;
  }

  asm.depsStallSource = [...new Set(getSourceRegsFiltered(asm))];
  asm.depsStallTarget = [...new Set(getTargetRegs(asm))];

  asm.depsSource = asm.depsStallSource.flatMap(expandRegister);
  asm.depsTarget = asm.depsStallTarget.flatMap(expandRegister);

  asm.depsSourceMask = getRegisterMask(asm.depsSource);
  if(asm.funcArgs && asm.funcArgs.length) {
    asm.depsArgMask = getRegisterMask(asm.funcArgs);
  }

  asm.depsTargetMask = getRegisterMask(asm.depsTarget);

  //console.log("Mask Src: ", asm.depsSourceMask.toString(2));
  //console.log("Mask Tgt: ", asm.depsTargetMask.toString(2));

  asm.depsStallSource = asm.depsStallSource
    .map(reg => reg.split(".")[0])
    .filter(reg => !STALL_IGNORE_REGS.includes(reg));

  asm.depsStallTarget = asm.depsStallTarget
    .map(reg => reg.split(".")[0])
    .filter(reg => !STALL_IGNORE_REGS.includes(reg));

  asm.depsStallSourceMask = getRegisterMask(asm.depsStallSource);
  asm.depsStallTargetMask = getRegisterMask(asm.depsStallTarget);

  asm.barrierMask = 0;
  for(const anno of asm.annotations) {
    if(anno.name === "Barrier") {
      asm.barrierMask |= state.getBarrierMask(anno.value);
    }
  }
}

/**
 * Scans blocks (e.g. if-condition), and collects data for all instructions in the block.
 * @param {ASM[]} asmList
 * @param {number} i current index
 */
export function asmInitBlockDep(asmList, i)
{
  const asm = asmList[i];
  // check if we are the start of a block (e.g. if-condition)
  if(!asm.opIsBranch || !asm.labelEnd) {
    return;
  }

  // Now scan the entire block and collect all r/r registers access.
  // also look out for jump/calls, since we cannot be sure about registers then.
  asm.depsBlockSourceMask = 0n;
  asm.depsBlockTargetMask = 0n;
  for(let j=i+1; j < asmList.length; ++j)
  {
    const asmNext = asmList[j];
    if(asmNext.label === asm.labelEnd) {
      break;
    }
    if(asmNext.opIsBranch) {
      asm.depsBlockSourceMask = REG_MASK_ALL;
      asm.depsBlockTargetMask = REG_MASK_ALL;
      break;
    }

    asm.depsBlockSourceMask |= asmNext.depsSourceMask;
    asm.depsBlockTargetMask |= asmNext.depsTargetMask;
  }
  //console.log("Block: ", i, asm.depsBlockSourceMask.toString(2), asm.depsBlockTargetMask.toString(2));
}

/**
 * Inits data necessary for further dependency analysis.
 * This mostly sets up the source/target registers for each instruction.
 * @param {ASMFunc} asmFunc
 */
export function asmInitDeps(asmFunc) {
  for(const asm of asmFunc.asm) {
    asmInitDep(asm);
  }
  for(const i in asmFunc.asm) {
    asmInitBlockDep(asmFunc.asm, parseInt(i));
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
    // Check for block-skips, See note in 'asmGetReorderIndices' for more info.
    if(asmPrev.labelEnd) {
       // @TODO: scan backwards to find the start label
    }

    return true;
  }

  if(asm.barrierMask & asmPrev.barrierMask) {
    return true;
  }

  // Don't reorder writes to RAM, this is an oversimplification.
  // For a more accurate check, the RAM location/size would need to be checked (if possible).
  /*const isStore = !asm.opIsLoad && asm.opIsStore;
  if(asm.opIsLoad || isStore) { // memory access can be ignored if it's not a load or store
    const isLoadPrev = asmPrev.opIsLoad;
    const isStorePrev = !isLoadPrev && asmPrev.opIsStore;

    // load cannot be put before a previous store (previous load is ok)
    if(asm.opIsLoad && isStorePrev) {
      return true;
    }
    // store cannot be put before a previous load or store
    //if(isStore && (isLoadPrev || isStorePrev)) {
    if(isStore && isLoadPrev) {
     return true;
    }
  }*/

  // check if any of our source registers is a destination of a previous instruction, and the reserve.
  // (otherwise our read would see a different value if reordered)
  // (otherwise out write could change what the previous instruction(s) reads)
  if((asmPrev.depsTargetMask & asm.depsSourceMask) !== 0n)return true;
  if((asmPrev.depsSourceMask & asm.depsTargetMask) !== 0n)return true;

  return false;
}

/**
 * Returns the indices where an instruction can be reordered to.
 * @param {ASM[]} asmList
 * @param {number} i
 * @return {number[]} array of indices
 */
export function asmGetReorderIndices(asmList, i)
{
  const asm = asmList[i];
  if(asm.type !== ASM_TYPE.OP || asm.opIsImmovable) {
    return [i];
  }

  // Scan ahead...
  let lastWrite = new Array(REG_INDEX_MAP.SIZE);
  let lastWriteMask = 0n;
  let lastReadMask = 0n;

  let pos = asmList.length;

  let f = i + 1;
  let isPastBranch = false;
  for(; f < asmList.length; ++f) {
    const asmNext = asmList[f];
    const amsPrevPrev = asmList[f-2];

    // stop at a branch with an already filled delay-slot,
    // or once we are past the delay-slot of a branch if it is empty.
    const isFilledBranch = asmNext.opIsBranch && !asmList[f+1]?.isNOP;
    isPastBranch = amsPrevPrev?.opIsBranch;

    // @TODO: wip, implement forward and backward scan for this:
    // if the branch we hit is part of a block (e.g. if condition), we usually cannot move past it.
    // However, it is possible under these conditions:
    // - all instructions in the block don't read/write to any of our target registers
    // - all instructions in the block don't write to any of our source registers
    // - the block contains no jumps/calls (since we cannot be sure about the registers)
    /*if(isPastBranch && amsPrevPrev.labelEnd)
    {
      // Note: jumps/calls are implicitly handled, they set all bits in the block-mask.
      const blockMaskRW = amsPrevPrev.depsBlockSourceMask | amsPrevPrev.depsBlockTargetMask;

      if((asm.depsTargetMask & blockMaskRW) === 0n
      && (asm.depsSourceMask & amsPrevPrev.depsBlockTargetMask) === 0n)
      {
        // it's safe to move past the block, try to find the end label and continue scanning there
        const endIndex = asmList.findIndex(asm => asm.label === amsPrevPrev.labelEnd);
        if(endIndex) {
          //console.log(asm.op, amsPrevPrev.labelEnd, endIndex);
          // @TODO: handle gaps
          f = endIndex;
          continue;
        }
      }
    }*/

    if(isFilledBranch || isPastBranch || checkAsmBackwardDep(asmNext, asm)) {
      pos = f;
      break;
    }

    // Remember the last write that occurs for each register, this is used to fall back if we stop at a read.
    for(const reg of asmNext.depsTarget) {
      lastWrite[REG_INDEX_MAP[reg]] = f;
    }
    lastWriteMask |= asmNext.depsTargetMask;
  }

  // even though we (may have) found a dependency, we still need to know if something tries to
  // read from one of our sources. Not doing so would prevent us from reordering the last write of a chain.
  let fRead = (isPastBranch ? f-2 : f)
  for(; fRead < asmList.length; ++fRead) {
    lastReadMask |= asmList[fRead].depsSourceMask;

    // branches/jumps needs special care, their target can make use of registers set before in code.
    // The function arguments (if known) are stored in a separate mask
    if(asmList[fRead].opIsBranch) {
      lastReadMask |= asmList[fRead].depsArgMask;
    }
  }

  // check if there was an instruction in between which wrote to one of our target registers.
  // if true, fall-back to that position (otherwise register would contain wrong value)
  const readWriteMask = lastReadMask & lastWriteMask;
  if((readWriteMask & asm.depsTargetMask) !== 0n) // avoid loop by checking mask first
  {
    for(const reg of asm.depsTarget) {
      let lastWritePos = lastWrite[REG_INDEX_MAP[reg]];
      if(lastWritePos && (lastReadMask & REG_MASK_MAP[reg]) !== 0n) {
        pos = Math.min(lastWritePos, pos);
      }
    }
  }

  const minMax = [0, pos-1];

  // collect all registers that where not overwritten by any instruction after us.
  // these need to be checked for writes in the backwards-scan.
  const writeCheckRegsMask = asm.depsTargetMask & ~lastWriteMask;

  // go backwards through all instructions before...
  for(let b=i-1; b >= 0; --b)
  {
    const asmPrev = asmList[b];

    if(asmList[b-1]?.opIsBranch // stop at the delay-slot of a branch, we cannot fill it backwards
      || checkAsmBackwardDep(asm, asmPrev)
      || (asmPrev.depsTargetMask & writeCheckRegsMask) !== 0n
    ) {
      minMax[0] = b+1;
      break;
    }
  }

  // @TODO:
  // min-max to array
  let res = [];
  for(let i = minMax[0]; i <= minMax[1]; ++i) {
    res.push(i);
  }
  return res;
}
/**
 * @param {ASMFunc} asmFunc
 */
export function asmScanDeps(asmFunc)
{
  for(let i = 0; i < asmFunc.asm.length; ++i) {
    const minMax = asmGetReorderIndices(asmFunc.asm, i);
    let min = Math.min(...minMax);
    let max = Math.max(...minMax);
    asmFunc.asm[i].debug.reorderLineMin = asmFunc.asm[min]?.debug.lineASM;
    asmFunc.asm[i].debug.reorderLineMax = asmFunc.asm[max]?.debug.lineASM;
  }
}
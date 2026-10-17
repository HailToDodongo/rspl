/**
* @copyright 2026 - Max Bebök
* @license Apache-2.0
*/
import { TYPE_ALIGNMENT, TYPE_ASM_DEF, TYPE_SIZE } from '../dataTypes/dataTypes';
import { LOAD_OPS_SCALAR, LOAD_OPS_VECTOR, STORE_OPS } from '../optimizer/asmScanDeps';
import {getAsmLabels} from '../writer/utils';
import state from '../state.js';

// vector loads/stores that have the "reg[element], offset(base)" syntax
const MEM_OPS_VECTOR = [...LOAD_OPS_VECTOR, ...STORE_OPS.filter(op => op.length === 3 && op.endsWith("v"))];
// scalar loads/stores always need a base register
const MEM_OPS_SCALAR = [...LOAD_OPS_SCALAR, ...STORE_OPS.filter(op => op.length === 2)];
const HAS_BASE_REG = /\(\$\w+\)$/;

const ELEMENT_MAP = {
  v: "",
  q0: "[0q]", q1: "[1q]",
  h0: "[0h]", h1: "[1h]", h2: "[2h]", h3: "[3h]",
  e0: "[0]", e1: "[1]", e2: "[2]", e3: "[3]",
  e4: "[4]", e5: "[5]", e6: "[6]", e7: "[7]",
};

/**
 * map lanes to armips "$v01.e0" => "$v01[0]"
 * @param {string} arg
 * @returns {string}
 */
function elementArg(arg)
{
  if(typeof arg !== "string" || !arg.startsWith("$") || !arg.includes("."))return arg;
  const [reg, element] = arg.split(".");
  const mapped = ELEMENT_MAP[element];
  if(mapped === undefined)state.throwError(`Unknown vector element '${element}' in '${arg}'!`);
  return reg + mapped;
}

function writeStateVar(stateVar, writeLines) {
  const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
  const byteSize = TYPE_SIZE[stateVar.varType] * arraySize;
  let align = TYPE_ALIGNMENT[stateVar.varType];
  if(stateVar.align !== 0) {
    align = stateVar.align;
  }

  const values = stateVar.value || [];

  if(align > 0) {
    writeLines(`    .align ${align}`);
  }

  if(values.length === 0) {
    writeLines(`    ${stateVar.varName}: .fill ${byteSize},0`);
  } else {
    const asmType = TYPE_ASM_DEF[stateVar.varType];
    const data = new Array(asmType.count * arraySize).fill(0);
    if(values.length > data.length) {
      state.throwError(`Too many initializers for '${stateVar.varName}' (${values.length} > ${data.length})`, stateVar);
    }
    for(let i=0; i<values.length; ++i) {
      data[i] = values[i];
    }
    writeLines(`    ${stateVar.varName}: .${asmType.typeArmips} ${data.join(", ")}`);
  }
  return byteSize;
}

function writeHeader(ast, functionsAsm, writeLines) 
{
  writeLines(['', ".rsp", '.create "dmem", 0x0000']);

  let totalSaveByteSize = 0;
  writeLines("  STATE_MEM_START:");

  for(const stateVar of ast.state) {
    if(stateVar.extern)continue;
    totalSaveByteSize += writeStateVar(stateVar, writeLines);
  }

  writeLines("  STATE_MEM_END:");

  if(ast.stateData.length > 0) {
    writeLines("");
    for(const tmpVar of ast.stateData) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLines);
    }
  }

  if(ast.stateBss.length > 0) {
    writeLines("");
    writeLines("  TEMP_STATE_MEM_START:");
    for(const tmpVar of ast.stateBss) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLines);
    }
    writeLines("  TEMP_STATE_MEM_END:");
  }

  writeLines([
    '.close', '',
    '.create "imem", 0x1000', 
    "OVERLAY_CODE_START:", 
    "RSPQ_Loop:",
    ""
  ]);

  return totalSaveByteSize;
}

/**
 * @param {string} label 
 * @param {number} offset 
 * @returns {string}
 */
function labelAddr(label, offset = 0)
{
  offset = parseInt(offset) || 0;
  if(offset != 0) {
    return `lo(${label} ${offset < 0 ? "" : "+"} ${offset})`;
  }
  return `lo(${label})`;
}

/**
 * @param {ASM} asm
 * @returns {string}
 */
function writeInstr(asm) 
{
  let args = "";
  const a = asm.args;
  if(a)
  {
    if(MEM_OPS_VECTOR.includes(asm.op)) {
      args = `${a[0]}[${a[1]}], ${a[2]}(${a[3]})`;
    } else if(MEM_OPS_SCALAR.includes(asm.op) && !HAS_BASE_REG.test(a[1])) {
      args = `${a[0]}, ${a[1]}($zero)`;
    } else {
      args = a.map(elementArg).join(", ");
    }
  }

  return getAsmLabels(asm) + asm.op + ' ' + args;
}

/** @type {Target} */
const target = {
  writeHeader,
  labelAddr,
  writeInstr
};
export default target;

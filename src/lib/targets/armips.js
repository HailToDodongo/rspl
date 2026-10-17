/**
* @copyright 2026 - Max Bebök
* @license Apache-2.0
*/
import { TYPE_ALIGNMENT, TYPE_ASM_DEF, TYPE_SIZE } from '../dataTypes/dataTypes';
import {getAsmLabels, getAttrLoaderLabel, getAttrPatchLabel, stringifyInstr} from '../writer/utils';

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
    return `%lo(${label} ${offset < 0 ? "" : "+"} ${offset})`;
  }
  return `%lo(${label})`;
}

/** @type {Target} */
const target = {
  writeHeader,
  labelAddr
};
export default target;

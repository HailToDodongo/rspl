/**
* @copyright 2026 - Max Bebök
* @license Apache-2.0
*/
import { TYPE_ALIGNMENT, TYPE_ASM_DEF, TYPE_SIZE } from '../dataTypes/dataTypes';
import {getAsmLabels, getAttrLoaderLabel, getAttrPatchLabel, stringifyInstr} from './utils';

function writeStateVar(stateVar, writeLine) {
  const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
  const byteSize = TYPE_SIZE[stateVar.varType] * arraySize;
  let align = TYPE_ALIGNMENT[stateVar.varType];
  if(stateVar.align !== 0) {
    align = stateVar.align;
  }

  const values = stateVar.value || [];

  if(align > 0) {
    writeLine(`    .align ${align}`);
  }

  if(values.length === 0) {
    writeLine(`    ${stateVar.varName}: .fill ${byteSize},0`);
  } else {
    const asmType = TYPE_ASM_DEF[stateVar.varType];
    const data = new Array(asmType.count * arraySize).fill(0);
    if(values.length > data.length) {
      state.throwError(`Too many initializers for '${stateVar.varName}' (${values.length} > ${data.length})`, stateVar);
    }
    for(let i=0; i<values.length; ++i) {
      data[i] = values[i];
    }
    writeLine(`    ${stateVar.varName}: .${asmType.typeArmips} ${data.join(", ")}`);
  }
  return byteSize;
}

export function writeArmipsHeader(ast, functionsAsm, writeLine, writeLines) 
{
  writeLines(['', ".rsp", '.create "dmem", 0x0000']);

  let totalSaveByteSize = 0;
  writeLine("  STATE_MEM_START:");

  for(const stateVar of ast.state) {
    if(stateVar.extern)continue;
    totalSaveByteSize += writeStateVar(stateVar, writeLine);
  }

  writeLine("  STATE_MEM_END:");

  if(ast.stateData.length > 0) {
    writeLine("");
    for(const tmpVar of ast.stateData) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLine);
    }
  }

  if(ast.stateBss.length > 0) {
    writeLine("");
    writeLine("  TEMP_STATE_MEM_START:");
    for(const tmpVar of ast.stateBss) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLine);
    }
    writeLine("  TEMP_STATE_MEM_END:");
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
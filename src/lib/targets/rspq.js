/**
* @copyright 2026 - Max Bebök
* @license Apache-2.0
*/
import {getAsmLabels, getAttrLoaderLabel, getAttrPatchLabel, stringifyInstr} from '../writer/utils';

export function writeHeader(ast, functionsAsm, writeLines, writeStateVar) 
{
  writeLines(["", ".data", "  RSPQ_BeginOverlayHeader"]);

  let commandList = [];
  for(const block of functionsAsm) {
    if(block.type === "command") {
      const name = block.nameOverride || block.name;
      commandList[block.resultType] = "    RSPQ_DefineCommand " + name + ", " + block.argSize;
    }
  }
  for(let i=0; i<commandList.length; ++i) {
    if(commandList[i] === undefined) {
      commandList[i] = "    RSPQ_DefineCommand RSPQ_Loop, 4";
    }
  }

  if(commandList.includes(undefined))state.throwError("Command list has gaps!", ast);

  writeLines(commandList);
  writeLines(["  RSPQ_EndOverlayHeader", ""]);

  let totalSaveByteSize = 0;

  const hasState = !!ast.state.find(v => !v.extern);
  if(hasState) {
    writeLines("  RSPQ_BeginSavedState");
    writeLines("    STATE_MEM_START:");

    for(const stateVar of ast.state) {
      if(stateVar.extern)continue;
      totalSaveByteSize += writeStateVar(stateVar, writeLines);
    }

    writeLines("    STATE_MEM_END:");
    writeLines("  RSPQ_EndSavedState");
  } else {
    writeLines("  RSPQ_EmptySavedState");
  }

  if(ast.stateData.length > 0) {
    writeLines("");
    for(const tmpVar of ast.stateData) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLines);
    }
  }

  if(ast.stateBss.length > 0) {
    writeLines("");
    writeLines(".bss");
    writeLines("  TEMP_STATE_MEM_START:");
    for(const tmpVar of ast.stateBss) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLines);
    }
    writeLines("  TEMP_STATE_MEM_END:");
  }

  writeLines(["", ".text", "OVERLAY_CODE_START:", ""]);

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

/**
 * @param {ASM} asm
 * @returns {string}
 */
function writeInstr(asm) {
  return getAsmLabels(asm) + asm.op + (asm.args.length ? (" " + asm.args.join(", ")) : "");
}

/** @type {Target} */
const target = {
  writeHeader,
  labelAddr,
  writeInstr,
  labelQueue: 'RSPQ_Loop'
};
export default target;

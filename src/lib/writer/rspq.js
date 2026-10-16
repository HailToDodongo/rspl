/**
* @copyright 2026 - Max Bebök
* @license Apache-2.0
*/
import {getAsmLabels, getAttrLoaderLabel, getAttrPatchLabel, stringifyInstr} from './utils';

export function writeRSPQHeader(ast, functionsAsm, writeLine, writeLines, writeStateVar) 
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
    writeLine("  RSPQ_BeginSavedState");
    writeLine("    STATE_MEM_START:");

    for(const stateVar of ast.state) {
      if(stateVar.extern)continue;
      totalSaveByteSize += writeStateVar(stateVar, writeLine);
    }

    writeLine("    STATE_MEM_END:");
    writeLine("  RSPQ_EndSavedState");
  } else {
    writeLine("  RSPQ_EmptySavedState");
  }

  if(ast.stateData.length > 0) {
    writeLine("");
    for(const tmpVar of ast.stateData) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLine);
    }
  }

  if(ast.stateBss.length > 0) {
    writeLine("");
    writeLine(".bss");
    writeLine("  TEMP_STATE_MEM_START:");
    for(const tmpVar of ast.stateBss) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLine);
    }
    writeLine("  TEMP_STATE_MEM_END:");
  }

  writeLines(["", ".text", "OVERLAY_CODE_START:", ""]);

  return totalSaveByteSize;
}
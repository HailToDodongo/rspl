/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import {TYPE_ALIGNMENT, TYPE_SIZE} from "./types/types";
import {normReg} from "./syntax/registers";

function stringifyInstr(parts) {
  if(!parts || parts.length === 0)return "";

  return parts[0]  + " " +
    parts.slice(1).map(normReg).join(", ")
}

function functionToASM(func)
{
  return func.name + ":\n"
    + func.asm.map(parts => "  " + stringifyInstr(parts)).join("\n")
}

export function writeASM(ast, functionsAsm)
{
  let text = "";
  let commandList = [];
  let savedState = "";
  let includes = "";

  for(const inc of ast.includes) {
    includes += `#include <${inc.replaceAll('"', '')}>\n`;
  }

  for(const stateVar of ast.state) {
    const byteSize = TYPE_SIZE[stateVar.varType] * stateVar.arraySize;
    const align = TYPE_ALIGNMENT[stateVar.varType];
    savedState += `    .align ${align}\n`;
    savedState += `    ${stateVar.varName}: .ds.b ${byteSize} \n`;
  }

  for(const block of functionsAsm)
  {
    if(["function", "command"].includes(block.type)) {
      text += functionToASM(block) + "\n\n";
    }

    if(block.type === "command") {
      commandList[block.resultType] = "    RSPQ_DefineCommand " + block.name + ", " + block.argSize;
    }
  }

  // commands have a gap, insert a dummy function to pad indices
  if(commandList.includes(undefined)) {
    text += "\nCMD_NOP:\n  jr ra\n  nop\n";
    for(let i = 0; i < commandList.length; i++) {
      if(commandList[i] === undefined) {
        commandList[i] = "    RSPQ_DefineCommand CMD_NOP, 0 ## Warning: Empty Command!";
      }
    }
  }

  return `## Auto-generated file, transpiled with RSPL
${includes}
.set noreorder
.set at

.data
  RSPQ_BeginOverlayHeader
${commandList.join("\n")}
  RSPQ_EndOverlayHeader

  ${savedState 
    ? "RSPQ_BeginSavedState\n"+savedState+"  RSPQ_EndSavedState"
    : "RSPQ_EmptySavedState"
  }

.text

${text}`;
}

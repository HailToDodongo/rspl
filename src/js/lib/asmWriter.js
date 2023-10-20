/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import {REGS_SCALAR} from "./syntax/registers";

// for whatever reason, the ASM uses "$" for vector regs, and no dollar for "normal" registers
function normReg(regName) {
  return REGS_SCALAR.includes(regName) ? regName.substring(1) : regName;
}

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

export function writeASM(asm)
{
  console.log("ASM", asm);
  let text = "";
  let commandList = [];
  let savedState = "";

  for(const block of asm) 
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
#include <rsp_queue.inc>
.set noreorder
.set at

.data
  RSPQ_BeginOverlayHeader
${commandList.join("\n")}
  RSPQ_EndOverlayHeader

  ${savedState 
    ? "RSPQ_BeginSavedState"+savedState+"RSPQ_EndSavedState"
    : "RSPQ_EmptySavedState"
  }

.text

${text}`;
}

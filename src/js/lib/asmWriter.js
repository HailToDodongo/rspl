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
  let data = "";
  let text = "";
  let savedState = "";

  for(const block of asm) 
  {
    if(block.type === "function") {
      text += functionToASM(block) + "\n";
    }
  }

  return `## Auto-generated file, transpiled with RSPL
#include <rsp_queue.inc>
.set noreorder
.set at

.data
  RSPQ_BeginOverlayHeader
  RSPQ_EndOverlayHeader

  ${savedState 
    ? "RSPQ_BeginSavedState"+savedState+"RSPQ_EndSavedState"
    : "RSPQ_EmptySavedState"
  }
${data}
.text

${text}`;
}

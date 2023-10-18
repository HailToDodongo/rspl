/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

// for whatever reason, the ASM uses "$" for vector regs, and no dollar for "normal" registers
const regNorm = {
  "$t0": "t0",
  "$t1": "t1",
};

function normReg(regName) {
  return regNorm[regName] || regName;
}

function functionToASM(func)
{
  return func.name + ":\n"
    + func.asm.map(parts => "  " + parts.map(x => normReg(x)).join(", ")).join("\n")

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

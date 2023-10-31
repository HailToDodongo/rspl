/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import {TYPE_ALIGNMENT, TYPE_SIZE} from "./types/types";
import state from "./state.js";
import {ASM_TYPE} from "./intsructions/asmWriter.js";
import {REGS_SCALAR} from "./syntax/registers.js";

function stringifyInstr(asm) {
  return asm.op + (asm.args.length ? (" " + asm.args.join(", ")) : "");
}

function functionToASM(func)
{
  let str = func.name + ":\n";
  for(const asm of func.asm) {
    switch (asm.type) {
      case ASM_TYPE.OP     : str += `  ${stringifyInstr(asm)}\n`; break;
      case ASM_TYPE.LABEL  : str += `  ${asm.label}:\n`;          break;
      case ASM_TYPE.COMMENT: str += `  ##${asm.comment}\n`;       break;
      default: state.throwError("Unknown ASM type: " + asm.type, asm);
    }
  }
  return str.trimEnd();
}

export function writeASM(ast, functionsAsm, config)
{
  state.func = "(ASM)";
  state.line = 0;

  let text = "";
  let commandList = [];
  let savedState = "";
  let includes = "";
  let postIncludes = "";

  for(const inc of ast.includes) {
    includes += `#include <${inc.replaceAll('"', '')}>\n`;
  }

  for(const inc of ast.postIncludes) {
    postIncludes += `#include <${inc.replaceAll('"', '')}>\n`;
  }

  for(const stateVar of ast.state) {
    if(stateVar.extern)continue;

    const byteSize = TYPE_SIZE[stateVar.varType] * stateVar.arraySize;
    const align = TYPE_ALIGNMENT[stateVar.varType];
    savedState += `    .align ${align}\n`;
    savedState += `    ${stateVar.varName}: .ds.b ${byteSize}\n`;
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
  text = text.trimEnd();

  // commands have a gap, insert a dummy function to pad indices
  if(commandList.includes(undefined)) {
    text += "\nCMD_NOP:\n  jr $ra\n  nop\n";
    for(let i = 0; i < commandList.length; i++) {
      if(commandList[i] === undefined) {
        commandList[i] = "    RSPQ_DefineCommand CMD_NOP, 0 ## Warning: Empty Command!";
      }
    }
  }

  if(!config.rspqWrapper) {
    return text;
  }

  // libdragon defines *some* registers without a "$", undo this to be consistent
  const regUndefs = REGS_SCALAR.map(reg => "#undef " + reg.substring(1)).join("\n");
  const regDefs = REGS_SCALAR.map((reg, i) => "#define " + reg.substring(1) + " $" + i)
    .filter((_, i) => i !== 1)
    .join("\n");
  const regHexDef = REGS_SCALAR.map((reg, i) => `.equ hex.${reg}, ${i}`).join("\n");

  return `## Auto-generated file, transpiled with RSPL
${includes}
.set noreorder
.set noat
.set nomacro

${regUndefs}
${regHexDef}

.data
  RSPQ_BeginOverlayHeader
${commandList.join("\n")}
  RSPQ_EndOverlayHeader

  ${savedState 
    ? "RSPQ_BeginSavedState\n"+savedState+"  RSPQ_EndSavedState"
    : "RSPQ_EmptySavedState"
  }

.text

${text}

${regDefs}

.set at
.set macro

${postIncludes}
`;
}

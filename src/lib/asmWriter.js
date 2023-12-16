/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {TYPE_ALIGNMENT, TYPE_SIZE} from "./dataTypes/dataTypes.js";
import state from "./state.js";
import {ASM_TYPE} from "./intsructions/asmWriter.js";
import {REGS_SCALAR} from "./syntax/registers.js";

/**
 * @param {ASM} asm
 * @returns {string}
 */
function stringifyInstr(asm) {
  return asm.op + (asm.args.length ? (" " + asm.args.join(", ")) : "");
}

/**
 * Writes the ASM of all functions and the AST into a string.
 * @param {AST} ast
 * @param {ASMFunc[]} functionsAsm
 * @param {RSPLConfig} config
 * @returns {ASMOutput}
 */
export function writeASM(ast, functionsAsm, config)
{
  state.func = "(ASM)";
  state.line = 0;

  /** @type {ASMOutput} */
  const res = {
    asm: "",
    debug: {lineMap: {}, lineDepMap: {}, lineOptMap: {}},
  };

  const writeLine = line => {
    res.asm += line + "\n";
    ++state.line;
  }
  const writeLines = lines => {
    res.asm += lines.join("\n") + "\n";
    state.line += lines.length;
  };

  writeLine("## Auto-generated file, transpiled with RSPL");

  for(const inc of ast.includes) {
    writeLine(`#include <${inc.replaceAll('"', '')}>`);
  }

  writeLines(["", ".set noreorder", ".set noat", ".set nomacro", ""]);

  REGS_SCALAR.forEach(reg => writeLine("#undef " + reg.substring(1)));
  REGS_SCALAR.forEach((reg, i) => writeLine(`.equ hex.${reg}, ${i}`));

  writeLines(["", ".data", "  RSPQ_BeginOverlayHeader"]);

  let commandList = [];
  for(const block of functionsAsm) {
    if(block.type === "command") {
      commandList[block.resultType] = "    RSPQ_DefineCommand " + block.name + ", " + block.argSize;
    }
  }

  if(commandList.includes(undefined))state.throwError("Command list has gaps!", ast);
  writeLines(commandList);
  writeLines(["  RSPQ_EndOverlayHeader", ""]);

  let totalSaveByteSize = 0;
  const hasState = !!ast.state.find(v => !v.extern);
  if(hasState) {
    writeLine("  RSPQ_BeginSavedState");

    for(const stateVar of ast.state) {
      if(stateVar.extern)continue;

      const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
      const byteSize = TYPE_SIZE[stateVar.varType] * arraySize;
      const align = TYPE_ALIGNMENT[stateVar.varType];

      writeLine(`    .align ${align}`);
      writeLine(`    ${stateVar.varName}: .ds.b ${byteSize}`);
      totalSaveByteSize += byteSize;
    }

    writeLine("  RSPQ_EndSavedState");
  } else {
    writeLine("  RSPQ_EmptySavedState");
  }

  const saveUsagePerc = totalSaveByteSize / 4096 * 100;
  state.logInfo(`Total state size: ${totalSaveByteSize} bytes (${saveUsagePerc.toFixed(2)}%)`);

  writeLines(["", ".text", ""]);

  if(!config.rspqWrapper) {
    state.line = 1;
    res.asm = "";
  }

  for(const block of functionsAsm) {
    if(!["function", "command"].includes(block.type))continue;

    writeLine(block.name + ":");

    for(const asm of block.asm)
    {
      if(!asm.debug.lineASM) {
        asm.debug.lineASM = state.line;
      } else {
        asm.debug.lineASMOpt = state.line;
        res.debug.lineOptMap[asm.debug.lineASM] = asm.debug.lineASMOpt;
      }

      const lineRSPL = asm.debug.lineRSPL;
      if(!res.debug.lineMap[lineRSPL])res.debug.lineMap[lineRSPL] = [];
      res.debug.lineMap[lineRSPL].push(asm.debug.lineASM);

      switch (asm.type) {
        case ASM_TYPE.INLINE:
        case ASM_TYPE.OP     : writeLine(`  ${stringifyInstr(asm)}`);break;
        case ASM_TYPE.LABEL  : writeLine(`  ${asm.label}:`);         break;
        case ASM_TYPE.COMMENT: writeLine(`  ##${asm.comment}`);      break;
        default: state.throwError("Unknown ASM type: " + asm.type, asm);
      }
    }

    for(const asm of block.asm)
    {
      if(!asm.debug.lineASMOpt)continue;
      //console.log(asm.debug.lineASM, [asm.debug.reorderLineMin, asm.debug.reorderLineMax]);
      res.debug.lineDepMap[asm.debug.lineASM] = [asm.debug.reorderLineMin, asm.debug.reorderLineMax];
    }
  }

  writeLine("");

  if(!config.rspqWrapper)return res;

  REGS_SCALAR.map((reg, i) => "#define " + reg.substring(1) + " $" + i)
    .filter((_, i) => i !== 1)
    .forEach(line => writeLine(line));

  writeLines(["", ".set at", ".set macro"]);

  for(const inc of ast.postIncludes) {
    writeLine(`#include <${inc.replaceAll('"', '')}>`);
  }

  return res;
}

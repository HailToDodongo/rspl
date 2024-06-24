/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {TYPE_ALIGNMENT, TYPE_ASM_DEF, TYPE_SIZE} from "./dataTypes/dataTypes.js";
import state from "./state.js";
import {ASM_TYPE} from "./intsructions/asmWriter.js";
import {REGS_SCALAR} from "./syntax/registers.js";
import {ANNOTATIONS, getAnnotationVal} from "./syntax/annotations.js";

/**
 * @param {ASM} asm
 * @returns {string}
 */
function stringifyInstr(asm) {
  return asm.op + (asm.args.length ? (" " + asm.args.join(", ")) : "");
}

/**
 * @param {string[]} incPaths
 */
function generateIncs(incPaths) {
  if(!incPaths)return [];
  const res = [];
  for(const inc of incPaths) {
    const pathNorm = inc.replaceAll('"', '');
    const wrap = pathNorm.startsWith(".") ? ['"', '"'] : ["<", ">"];

    res.push('#include ' + wrap[0] + pathNorm + wrap[1]);
  }
  return res;
}

function alignToExp(align) {
  const alignPow = Math.log2(align);
  if(!Number.isInteger(alignPow)) {
    state.throwError(`Invalid align value '${align}', must be a power of 2`);
  }
  return alignPow;
}

function writeStateVar(stateVar, writeLine) {
  const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
  const byteSize = TYPE_SIZE[stateVar.varType] * arraySize;
  let align = TYPE_ALIGNMENT[stateVar.varType];
  if(stateVar.align !== 0) {
    // align is stored in bytes, we need to convert it to 2^x
    align = alignToExp(stateVar.align);

  }

  const values = stateVar.value || [];

  if(align > 0) {
    writeLine(`    .align ${align}`);
  }

  if(values.length === 0) {
    writeLine(`    ${stateVar.varName}: .ds.b ${byteSize}`);
  } else {
    const asmType = TYPE_ASM_DEF[stateVar.varType];
    const data = new Array(asmType.count * arraySize).fill(0);
    if(values.length > data.length) {
      state.throwError(`Too many initializers for '${stateVar.varName}' (${values.length} > ${data.length})`, stateVar);
    }
    for(let i=0; i<values.length; ++i) {
      data[i] = values[i];
    }
    writeLine(`    ${stateVar.varName}: .${asmType.type} ${data.join(", ")}`);
  }
  return byteSize;
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
    debug: {lineMap: {}, lineDepMap: {}, lineOptMap: {}, lineCycleMap: {}, lineStallMap: {}},
    sizeDMEM: 0,
    sizeIMEM: 0,
  };

  const writeLine = line => {
    res.asm += line + "\n";
    ++state.line;
  }
  const writeLines = lines => {
    if(lines.length === 0)return;
    res.asm += lines.join("\n") + "\n";
    state.line += lines.length;
  };

  writeLine("## Auto-generated file, transpiled with RSPL");

  if(ast.defines) {
    for(const [name, def] of Object.entries(ast.defines)) {
      writeLine(`#define ${name} ${def.value}`);
    }
  }

  const preIncs = generateIncs(ast.includes);
  for(const inc of preIncs)writeLine(inc);

  writeLines(["", ".set noreorder", ".set noat", ".set nomacro", ""]);

  REGS_SCALAR.forEach(reg => writeLine("#undef " + reg.substring(1)));
  REGS_SCALAR.forEach((reg, i) => writeLine(`.equ hex.${reg}, ${i}`));
  writeLine("#define vco 0");
  writeLine("#define vcc 1");
  writeLine("#define vce 2");

  writeLines(["", ".data", "  RSPQ_BeginOverlayHeader"]);

  let commandList = [];
  for(const block of functionsAsm) {
    if(block.type === "command") {
      commandList[block.resultType] = "    RSPQ_DefineCommand " + block.name + ", " + block.argSize;
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
  let totalTextSize = 0;

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

  if(ast.tempState.length > 0) {
    writeLine("");
    writeLine("  TEMP_STATE_MEM_START:");
    for(const tmpVar of ast.tempState) {
      if(tmpVar.extern)continue;
      totalSaveByteSize += writeStateVar(tmpVar, writeLine);
    }
    writeLine("  TEMP_STATE_MEM_END:");
  }

  writeLines(["", ".text", "OVERLAY_CODE_START:", ""]);

  if(!config.rspqWrapper) {
    state.line = 1;
    res.asm = "";
  }

  for(const block of functionsAsm) {
    if(!["function", "command"].includes(block.type))continue;
    if(block.asm.length === 0)continue;

    const align = getAnnotationVal(block.annotations, ANNOTATIONS.Align, 0) || 0;
    if(align)writeLine(`.align ${alignToExp(align)}`);

    writeLine(block.name + ":");

    for(const asm of block.asm)
    {
      // Debug Information
      if(!asm.debug.lineASM) {
        asm.debug.lineASM = state.line;
      } else {
        asm.debug.lineASMOpt = state.line;
        res.debug.lineOptMap[asm.debug.lineASM] = asm.debug.lineASMOpt;
      }

      const lineRSPL = asm.debug.lineRSPL;
      if(!res.debug.lineMap[lineRSPL])res.debug.lineMap[lineRSPL] = [];
      res.debug.lineMap[lineRSPL].push(asm.debug.lineASM);

      if(asm.debug.cycle) {
        res.debug.lineCycleMap[asm.debug.lineASMOpt] = asm.debug.cycle;
        res.debug.lineStallMap[asm.debug.lineASMOpt] = asm.debug.stall;
      }

      let debugInfo = asm.barrierMask
        ? " ## Barrier: 0x" + asm.barrierMask.toString(16).toUpperCase()
        : "";

      if(asm.funcArgs && asm.funcArgs.length) {
        debugInfo += " ## Args: " + asm.funcArgs.join(", ");
      }

      // ASM Text output
      switch (asm.type) {
        case ASM_TYPE.INLINE:
        case ASM_TYPE.OP     : writeLine(`  ${stringifyInstr(asm)}${debugInfo}`);break;
        case ASM_TYPE.LABEL  : writeLine(`  ${asm.label}:`);         break;
        case ASM_TYPE.COMMENT: writeLine(`  ##${asm.comment}`);      break;
        default: state.throwError("Unknown ASM type: " + asm.type, asm);
      }

      totalTextSize += asm.type === ASM_TYPE.OP ? 4 : 0;
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

  writeLine("OVERLAY_CODE_END:");
  writeLine("");

  REGS_SCALAR.map((reg, i) => "#define " + reg.substring(1) + " $" + i)
    .filter((_, i) => i !== 1)
    .forEach(line => writeLine(line));

  writeLines(["", ".set at", ".set macro"]);

  const postIncs = generateIncs(ast.postIncludes);
  for(const inc of postIncs)writeLine(inc);

  res.sizeDMEM = totalSaveByteSize;
  res.sizeIMEM = totalTextSize;
  return res;
}

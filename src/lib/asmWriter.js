/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {TYPE_ALIGNMENT, TYPE_ASM_DEF, TYPE_SIZE} from "./dataTypes/dataTypes.js";
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
 * @param {string[]} incPaths
 */
function generateIncs(incPaths) {
  const res = [];
  for(const inc of incPaths) {
    const pathNorm = inc.replaceAll('"', '');
    const wrap = pathNorm.startsWith(".") ? ['"', '"'] : ["<", ">"];

    res.push('#include ' + wrap[0] + pathNorm + wrap[1]);
  }
  return res;
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

  if(commandList.includes(undefined))state.throwError("Command list has gaps!", ast);
  writeLines(commandList);
  writeLines(["  RSPQ_EndOverlayHeader", ""]);

  let totalSaveByteSize = 660; // libdragon default (@TODO get this from somewhere)
  let totalTextSize = 616; // libdragon default (@TODO get this from somewhere)

  const hasState = !!ast.state.find(v => !v.extern);
  if(hasState) {
    writeLine("  RSPQ_BeginSavedState");

    for(const stateVar of ast.state) {
      if(stateVar.extern)continue;

      const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
      const byteSize = TYPE_SIZE[stateVar.varType] * arraySize;
      let align = TYPE_ALIGNMENT[stateVar.varType];
      if(stateVar.align !== 0) {
        // align is stored in bytes, we need to convert it to 2^x
        align = Math.log2(stateVar.align);
        if(!Number.isInteger(align)) {
          state.throwError(`Invalid align value '${stateVar.align}', must be a power of 2`, ast);
        }
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
          state.throwError(`Too many initializers for '${stateVar.varName}' (${values.length} > ${data.length})`, ast);
        }
        for(let i=0; i<values.length; ++i) {
          data[i] = values[i];
        }
        writeLine(`    ${stateVar.varName}: .${asmType.type} ${data.join(", ")}`);
      }

      totalSaveByteSize += byteSize;
    }

    writeLine("  RSPQ_EndSavedState");
  } else {
    writeLine("  RSPQ_EmptySavedState");
  }

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

      // ASM Text output
      switch (asm.type) {
        case ASM_TYPE.INLINE:
        case ASM_TYPE.OP     : writeLine(`  ${stringifyInstr(asm)}`);break;
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

  REGS_SCALAR.map((reg, i) => "#define " + reg.substring(1) + " $" + i)
    .filter((_, i) => i !== 1)
    .forEach(line => writeLine(line));

  writeLines(["", ".set at", ".set macro"]);

  const postIncs = generateIncs(ast.postIncludes);
  for(const inc of postIncs)writeLine(inc);

  const saveUsagePerc = totalSaveByteSize / 4096 * 100;
  state.logInfo(`Total state size: ${totalSaveByteSize} bytes (${saveUsagePerc.toFixed(2)}%)`);
  const textUsagePerc = totalTextSize / 4096 * 100;
  state.logInfo(`Total text size: ${totalTextSize} bytes (${textUsagePerc.toFixed(2)}%)`);

  return res;
}

/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {TYPE_ALIGNMENT, TYPE_ASM_DEF, TYPE_SIZE} from "../dataTypes/dataTypes.js";
import state from "../state.js";
import {ASM_TYPE} from "../intsructions/asmWriter.js";
import {REGS_SCALAR} from "../syntax/registers.js";
import {ANNOTATIONS, getAnnotationVal} from "../syntax/annotations.js";
import { writeMagmaHeader } from "./magma.js";
import {getAsmLabels, getAttrLoaderLabel, getAttrPatchLabel, stringifyInstr} from './utils';
import { writeRSPQHeader } from "./rspq.js";
import { writeArmipsHeader } from "./armips.js";

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

function writeStateVar(stateVar, writeLines) {
  const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
  const byteSize = TYPE_SIZE[stateVar.varType] * arraySize;
  let align = TYPE_ALIGNMENT[stateVar.varType];
  if(stateVar.align !== 0) {
    // align is stored in bytes, we need to convert it to 2^x
    align = alignToExp(stateVar.align);

  }

  const values = stateVar.value || [];

  if(align > 0) {
    writeLines(`    .align ${align}`);
  }

  if(values.length === 0) {
    writeLines(`    ${stateVar.varName}: .ds.b ${byteSize}`);
  } else {
    const asmType = TYPE_ASM_DEF[stateVar.varType];
    const data = new Array(asmType.count * arraySize).fill(0);
    if(values.length > data.length) {
      state.throwError(`Too many initializers for '${stateVar.varName}' (${values.length} > ${data.length})`, stateVar);
    }
    for(let i=0; i<values.length; ++i) {
      data[i] = values[i];
    }
    writeLines(`    ${stateVar.varName}: .${asmType.type} ${data.join(", ")}`);
  }
  return byteSize;
}

/**
 * Writes the ASM of all functions and the AST into a string.
 * @param {AST} ast
 * @param {ASMFunc[]} functionsAsm
 * @returns {ASMOutput}
 */
export function writeASM(ast, functionsAsm)
{
  const config = state.config;
  state.func = "(ASM)";
  state.line = 0;

  /** @type {ASMOutput} */
  const res = {
    asm: "",
    debug: {lineMap: {}, lineDepMap: {}, lineOptMap: {}, lineCycleMap: {}, lineStallMap: {}},
    sizeDMEM: 0,
    sizeIMEM: 0,
  };

  const writeLines = lines => {
    if(Array.isArray(lines)) {
      if(lines.length === 0)return;
      res.asm += lines.join("\n") + "\n";
      state.line += lines.length;
    } else {
      res.asm += lines + "\n";
      ++state.line;
    }
  };

  const comment = config.armips 
     ? (txt => " // " + txt)
     : (txt => " ## " + txt);

  const define = config.armips
    ? (name, v) => `${name} equ ${v ?? '1'}`.trim()
    : (name, v) => `#define ${name} ${v}`.trim();

  writeLines(comment("Auto-generated file, transpiled with RSPL").trim());

  if(ast.defines) {
    for(const [name, def] of Object.entries(ast.defines)) {
      if (!def.value) {
        writeLines(define(name, ''));
      } else {
        writeLines(define(name, def.value));
      }
    }
  }

  const preIncs = generateIncs(ast.includes);
  for(const inc of preIncs)writeLines(inc);

  if(!config.armips) {
    writeLines(["", ".set noreorder", ".set noat", ".set nomacro", ""]);

    REGS_SCALAR.forEach(reg => writeLines("#undef " + reg.substring(1)));
    REGS_SCALAR.forEach((reg, i) => writeLines(`.equ hex.${reg}, ${i}`));
    writeLines("#define vco 0");
    writeLines("#define vcc 1");
    writeLines("#define vce 2");
  }

  let totalSaveByteSize = 0;
  let totalTextSize = 0;

  if(config.magma) {
    totalSaveByteSize += writeMagmaHeader(ast, functionsAsm, writeLines, writeStateVar);
  } else if(config.armips) {
    totalSaveByteSize += writeArmipsHeader(ast, functionsAsm, writeLines, writeStateVar);
  } else {
    totalSaveByteSize += writeRSPQHeader(ast, functionsAsm, writeLines, writeStateVar);
  }

  if(!config.rspqWrapper) {
    state.line = 1;
    res.asm = "";
  }

  const writeFunction = block => {
    if(block.asm.length === 0)return;

    const align = getAnnotationVal(block.annotations, ANNOTATIONS.Align, 0) || 0;
    if (align) writeLines(`.align ${alignToExp(align)}`);

    if (block.type !== "shader") writeLines(block.name + ":");

    let lastAsm = block.asm[0] || null;
    for (const asm of block.asm) {
      // Debug Information
      if (!asm.debug.lineASM) {
        asm.debug.lineASM = state.line;
      } else {
        asm.debug.lineASMOpt = state.line;
        res.debug.lineOptMap[asm.debug.lineASM] = asm.debug.lineASMOpt;
      }

      const lineRSPL = asm.debug.lineRSPL;
      if (!res.debug.lineMap[lineRSPL]) res.debug.lineMap[lineRSPL] = [];
      res.debug.lineMap[lineRSPL].push(asm.debug.lineASM);

      if (asm.debug.cycle) {
        res.debug.lineCycleMap[asm.debug.lineASMOpt] = asm.debug.cycle;
        res.debug.lineStallMap[asm.debug.lineASMOpt] = asm.debug.stall;
      }

      let debugInfo = '';

      if (config.debugInfo) {
        if (asm.debug.lineRSPL) {
          let cycleStr = '     ^';
          let cycleDiff = asm.debug.cycle - lastAsm.debug.cycle;
          if (cycleDiff !== 0) {
            let stars = '';
            if (cycleDiff > 1) {
              stars = '*'.repeat(cycleDiff - 1);
            }
            cycleStr = (stars + asm.debug.cycle.toString()).padStart(6, ' ');
          }

          debugInfo += comment(`L:${asm.debug.lineRSPL.toString().padEnd(4, ' ')} | ${cycleStr} | ${state.sourceLines[asm.debug.lineRSPL - 1] || ''}`);
        }

        if (asm.funcArgs && asm.funcArgs.length) {
          debugInfo += comment("Args: " + asm.funcArgs.join(", "));
        }

        if (asm.barrierMask) {
          debugInfo += comment("Barrier: 0x" + asm.barrierMask.toString(16).toUpperCase());
        }
      }

      let tag = '';
      for (const ann of asm.annotations) {
        if (ann.name === ANNOTATIONS.Tag) {
          tag = `TAG_${ann.value}: `;
          break;
        }
      }

      // ASM Text output
      switch (asm.type) {
        case ASM_TYPE.INLINE:
        case ASM_TYPE.OP: writeLines(`  ${tag}${stringifyInstr(asm).padEnd(debugInfo ? 50 : 0, ' ')}${debugInfo}`); break;
        case ASM_TYPE.LABEL: writeLines(`  ${tag}${asm.label}:`); break;
        default: state.throwError("Unknown ASM type: " + asm.type, asm);
      }

      totalTextSize += asm.type === ASM_TYPE.OP ? 4 : 0;
      if (asm.type === ASM_TYPE.OP) {
        lastAsm = asm;
      }
    }

    for (const asm of block.asm) {
      if (!asm.debug.lineASMOpt) continue;
      //console.log(asm.debug.lineASM, [asm.debug.reorderLineMin, asm.debug.reorderLineMax]);
      res.debug.lineDepMap[asm.debug.lineASM] = [asm.debug.reorderLineMin, asm.debug.reorderLineMax];
    }
  }

  if (config.magma) {
    const shaderFunctions = functionsAsm.filter(fn => fn.type === "shader");
    for (const block of shaderFunctions) {
      writeFunction(block);
    }
  }

  const regularFunctionsAsm = functionsAsm.filter(fn => ["function", "command"].includes(fn.type));
  for (const block of regularFunctionsAsm) {
    writeFunction(block);
  }

  writeLines("");

  if(!config.rspqWrapper)return res;

  if(config.magma) {
    writeLines("MgEndShader");
    writeLines("");
  } else if(config.armips) {  
    writeLines("OVERLAY_CODE_END:");
    writeLines(".close");
    writeLines("");
  } else {
    writeLines("OVERLAY_CODE_END:");
    writeLines("");
  }

  if(!config.armips) {
    REGS_SCALAR.map((reg, i) => "#define " + reg.substring(1) + " $" + i)
      .filter((_, i) => i !== 1)
      .forEach(line => writeLines(line));

    writeLines(["", ".set at", ".set macro"]);
  }

  const postIncs = generateIncs(ast.postIncludes);
  for(const inc of postIncs)writeLines(inc);

  res.sizeDMEM = totalSaveByteSize;
  res.sizeIMEM = totalTextSize;
  return res;
}

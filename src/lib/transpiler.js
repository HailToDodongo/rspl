/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import { ast2asm } from "./ast2asm";
import { writeASM } from "./asmWriter";
import {astNormalizeFunctions} from "./astNormalize";
import nearly from "nearley";
import grammarDef from "./grammar.cjs";
import state from "./state.js";
import {normalizeASM} from "./asmNormalize.js";
import {asmOptimize, asmOptimizePattern} from "./optimizer/asmOptimizer.js";
import {asmInitDeps, asmScanDeps} from "./optimizer/asmScanDeps.js";
import {evalFunctionCost} from "./optimizer/eval/evalCost.js";
import {preprocess, stripComments} from "./preproc/preprocess.js";
import {validateMemory} from "./memory/memoryValidator.js";

const grammar = nearly.Grammar.fromCompiled(grammarDef);
/**
 * @param {RSPLConfig} config
 */
function normalizeConfig(config)
{
  if(config.rspqWrapper === undefined)config.rspqWrapper = true;
  if(config.optimize    === undefined)config.optimize = false;
}

/**
 * @param {string} source
 * @param {RSPLConfig} config
 * @param {function} updateCb
 */
export async function transpileSource(source, config, updateCb = undefined)
{
  source = stripComments(source);
  const parser = new nearly.Parser(grammar);

  console.time("Preprocessor");
  const defines = {};
  if(config.defines) {
    for(const [key, value] of Object.entries(config.defines)) {
      defines[key] = {
        regex: new RegExp(`\\b${key}\\b`, "g"),
        value: value
      };
    }
  }

  source = preprocess(source, defines, config.fileLoader);
  console.timeEnd("Preprocessor");

  //console.time("parser");
  const astList = parser.feed(source);
  //console.timeEnd("parser");

  if(astList.results.length > 1) {
    throw Error("Warning: ambiguous syntax!");
  }
  const ast = astList.results[0];
  ast.defines = defines;

  try {
  return await transpile(ast, updateCb, config);
  } catch (e) {
    if(e.message.includes("Error in") && e.message.includes("line ")) {
      // add surrounding lines to error message
      const lineCount = 3;
      const lines = source.split("\n");
      const line = parseInt(e.message.match(/line (\d+)/)[1]);
      const start = Math.max(0, line - lineCount);
      const end = Math.min(lines.length, line + lineCount);
      const context = lines.slice(start, end)
        .map((l, i) =>
          `${line === (start+i+1) ? '>' : ' '}${(start + i + 1).toString().padStart(4, ' ')}: ${l}`
        )
        .join("\n");

      e.message += "\n\nSource:\n" + context + "\n";
    }
    throw e;
  }
}

/**
 * @param {AST} ast
 * @param {function} updateCb
 * @param {RSPLConfig} config
 */
export async function transpile(ast, updateCb, config = {})
{
  state.reset();
  normalizeConfig(config);

  validateMemory(ast.state, ast.tempState);
  ast.functions = astNormalizeFunctions(ast);
  const functionsAsm = ast2asm(ast);

  for(const func of functionsAsm) {
    normalizeASM(func);
  }

  let debugUnopt = {};

  const generateASM = () => {
    const {asm, debug, sizeDMEM, sizeIMEM} = writeASM(ast, functionsAsm, config);
  //console.timeEnd("writeASM");

    const usageImemPercent = sizeDMEM / 4096 * 100;
    state.logInfo(`Total state size: ${sizeDMEM} bytes (${usageImemPercent.toFixed(2)}%)`);
    const useageDmemPercent = sizeIMEM / 4096 * 100;
    state.logInfo(`Total text size: ${sizeIMEM} bytes (${useageDmemPercent.toFixed(2)}%)`);

    debug.lineDepMap = debugUnopt.lineDepMap;
    return {
      asm: asm.trimEnd(),
      asmUnoptimized,
      debug,
      warn: state.outWarn,
      info: state.outInfo,
      sizeDMEM, sizeIMEM,
    };
  };

  let asmUnoptimized = "";
  if(config.optimize)
  {
    // pre-generate the first version of the ASM to get line numbers
    for(const func of functionsAsm)
    {
      writeASM(ast, functionsAsm, config);
      asmInitDeps(func);
      asmScanDeps(func); // debugging only
    }

    const resUnopt = writeASM(ast, functionsAsm, config);
    asmUnoptimized = resUnopt.asm;
    debugUnopt = resUnopt.debug;

    for(const func of functionsAsm)
    {
      asmOptimizePattern(func);
      if(func.asm.length > 0) {
        asmInitDeps(func);

        console.time("asmOptimize");
        await asmOptimize(func, (bestFunc) => {
          if(updateCb)updateCb(generateASM());
        }, config);
        console.timeEnd("asmOptimize");

        asmScanDeps(func); // debugging only
        func.cyclesAfter = evalFunctionCost(func);
      } else {
        func.cyclesAfter = 0;
      }
    }

    console.log("==== Optimization Overview ====");
    let longestFuncName = functionsAsm.reduce((a, b) => a.name.length > b.name.length ? a : b).name.length;
    for(const func of functionsAsm) {
      console.log(`- ${func.name.padEnd(longestFuncName, ' ')}: ${func.cyclesBefore.toString().padStart(4, ' ')}  -> ${func.cyclesAfter.toString().padStart(4, ' ')} cycles`);
    }
    console.log("===============================");
  }

  return generateASM();
}

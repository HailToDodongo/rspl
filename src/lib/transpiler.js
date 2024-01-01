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

const grammar = nearly.Grammar.fromCompiled(grammarDef);

/**
 * @param {RSPLConfig} config
 */
function normalizeConfig(config)
{
  if(config.rspqWrapper === undefined)config.rspqWrapper = true;
  if(config.optimize    === undefined)config.optimize = false;
}

function stripComments(source) {
  return source
    .replaceAll(/\/\/.*$/gm, "")
    .replace(/\/\*[\s\S]*?\*\//g, match => {
      const newlineCount = match.split('\n').length - 1;
      return '\n'.repeat(newlineCount);
    });
}

/**
 * @param {string} source
 * @param {RSPLConfig} config
 */
export function transpileSource(source, config)
{
  source = stripComments(source);
  const parser = new nearly.Parser(grammar);

  //console.time("parser");
  const astList = parser.feed(source);
  //console.timeEnd("parser");

  if(astList.results.length > 1) {
    throw Error("Warning: ambiguous syntax!");
  }
  return transpile(astList.results[0], config);
}

/**
 * @param {AST} ast
 * @param {RSPLConfig} config
 */
export function transpile(ast, config = {})
{
  state.reset();
  normalizeConfig(config);

  ast.functions = astNormalizeFunctions(ast);
  const functionsAsm = ast2asm(ast);

  for(const func of functionsAsm) {
    normalizeASM(func);
  }

  let asmUnoptimized = "";
  if(config.optimize)
  {
    asmUnoptimized = writeASM(ast, functionsAsm, config).asm;
    for(const func of functionsAsm) {
      asmOptimizePattern(func);

      //console.time("asmInitDeps");
      asmInitDeps(func);
      asmScanDeps(func); // debugging only
      //console.timeEnd("asmInitDeps");
      console.time("asmOptimize");
      asmOptimize(func);
      console.timeEnd("asmOptimize");

      evalFunctionCost(func);
    }
  }

  //console.time("writeASM");
  const {asm, debug} = writeASM(ast, functionsAsm, config);
  //console.timeEnd("writeASM");

  return {
    asm: asm.trimEnd(),
    asmUnoptimized,
    debug,
    warn: state.outWarn,
    info: state.outInfo,
  };
}

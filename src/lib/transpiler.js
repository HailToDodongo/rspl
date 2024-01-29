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
import {preprocess} from "./preproc/preprocess.js";

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
    .replaceAll(/#define\s*[\n()\[\]]/gm, "")
    .replace(/\/\*[\s\S]*?\*\//g, match => {
      const newlineCount = match.split('\n').length - 1;
      return '\n'.repeat(newlineCount);
    });
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
  source = preprocess(source);
  console.timeEnd("Preprocessor");

  //console.time("parser");
  const astList = parser.feed(source);
  //console.timeEnd("parser");

  if(astList.results.length > 1) {
    throw Error("Warning: ambiguous syntax!");
  }
  return await transpile(astList.results[0], updateCb, config);
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

  ast.functions = astNormalizeFunctions(ast);
  const functionsAsm = ast2asm(ast);

  for(const func of functionsAsm) {
    normalizeASM(func);
  }

  const generateASM = () => {
    const {asm, debug} = writeASM(ast, functionsAsm, config);
  //console.timeEnd("writeASM");

    return {
      asm: asm.trimEnd(),
      asmUnoptimized,
      debug,
      warn: state.outWarn,
      info: state.outInfo,
    };
  };

  let asmUnoptimized = "";
  if(config.optimize)
  {
    asmUnoptimized = writeASM(ast, functionsAsm, config).asm;
    for(const func of functionsAsm) {
      asmOptimizePattern(func);

      //console.time("asmInitDeps");
      asmInitDeps(func);

      //console.timeEnd("asmInitDeps");
      console.time("asmOptimize");
      await asmOptimize(func, (bestFunc) => {
        if(updateCb)updateCb(generateASM());
      }, config);
      console.timeEnd("asmOptimize");

      asmScanDeps(func); // debugging only
      evalFunctionCost(func);
    }
  }

  return generateASM();
}

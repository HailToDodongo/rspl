/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import { ast2asm } from "./ast2asm";
import { writeASM } from "./asmWriter";
import {astNormalizeFunctions} from "./astNormalize";
import nearly from "nearley";
import grammarDef from "./grammar.cjs";
import state from "./state.js";

function normalizeConfig(config)
{
  if(config.rspqWrapper === undefined)config.rspqWrapper = true;
  if(config.optimize    === undefined)config.optimize = true;
}

export function transpileSource(source, config)
{
  const parser = new nearly.Parser(nearly.Grammar.fromCompiled(grammarDef));
  const astList = parser.feed(source);
  if(astList.results.length > 1) {
    throw Error("Warning: ambiguous syntax!");
  }
  return transpile(astList.results[0], config);
}

export function transpile(ast, config = {})
{
  state.reset();
  normalizeConfig(config);
  //console.log("AST", ast);

  ast.functions = astNormalizeFunctions(ast);

  // @TODO: optimize AST
  
  const functionsAsm = ast2asm(ast);
  
  // @TODO: optimize ASM
  
  return {
    asm: writeASM(ast, functionsAsm, config),
    warn: state.outWarn,
  };
}

/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import { ast2asm } from "./ast2asm";
import { writeASM } from "./asmWriter";
import { astNormalize } from "./astNormalize";

export function transpile(ast)
{
  console.log("AST", ast);

  ast.functions = astNormalize(ast.functions);

  // @TODO: optimize AST
  
  const asm = ast2asm(ast);
  
  // @TODO: optimize ASM
  
  return writeASM(asm);
}

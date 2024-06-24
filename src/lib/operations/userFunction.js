/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import state from "../state.js";
import {asm, asmFunction, asmNOP} from "../intsructions/asmWriter.js";
import opsScalar from "./scalar.js";

/**
 * @param {string} name
 * @param {ASTFuncArg[]} args
 * @returns {ASM[]}
 */
export function callUserFunction(name, args)
{
  let userFunc = state.getFunction(name);
  if(!userFunc) {
    if(!state.varExists(name)) {
      state.throwError("Function "+name+" not known!");
    }
    userFunc = {
      name: state.getVarReg(name),
      args: [],
      isRelative: false
    };
  }

  const res = [];

  if(userFunc.args.length !== args.length) {
    state.throwError(`Function ${name} expects ${userFunc.args.length} arguments, got ${args.length}!`, args);
  }

  for(let i = 0; i < args.length; ++i) {
    const argUser = args[i];
    const argDef = userFunc.args[i];
    if(argUser.type === "num") {
      res.push(...opsScalar.loadImmediate(argDef.reg, argUser.value));
    } else {
      const argVar = state.getRequiredVar(argUser.value, "arg" + i);
      if(argVar.type !== argDef.type) {
        state.throwError(`Function ${name} expects argument ${i} to be of type ${argDef.type}, got ${argVar.type}!`, argUser);
      }
      if(argVar.reg !== argDef.reg) {
        state.throwError(`Function ${name} expects argument ${i} to be in register ${argDef.reg}, got ${argVar.reg}!`, argUser);
      }
    }
  }
  let isRelative = state.getAnnotations("Relative").length > 0;
  if(userFunc.isRelative) {
    isRelative = true;
  }

  const regsArg = userFunc.args.map(arg => arg.reg);
  res.push(asmFunction(userFunc.name, regsArg, isRelative), asmNOP());
  return res;
}
/**
* @copyright 2024 - Max Bebök
* @license Apache-2.0
*/

import state from "../state.js";
import {TYPE_ALIGNMENT, TYPE_SIZE} from "../dataTypes/dataTypes.js";

const STATE_NAMES = ["state", "data", "bss"];

/**
 * validates state memory
 * @param {AST} ast
 * @param {Array<{name: string; vars: ASTState[]}>} states
 */
export const validateMemory = (ast, states) =>
{
  state.func = "state";

  const names = states.map(s => s.name).flat();
  if((new Set(names)).size !== names.length) {
    state.throwError("A type of state can only appear once!", names);
  }

  ast.state = [];
  ast.stateBss = [];
  ast.stateData = [];

  for(const s of states) {
    switch(s.name) {
      case 'state': ast.state = s.vars; break;
      case 'data': ast.stateData = s.vars; break;
      case 'temp_state':
      case 'bss': ast.stateBss = s.vars; break;
      default:
        state.throwError(`Invalid state name: ${s.name}, must be one of: ${STATE_NAMES.join(", ")}`);
      break;
    }
  }

  let stateVars = [...ast.state, ...ast.stateBss, ...ast.stateData]
    .filter(v => !v.extern);

  for(const m of ast.stateBss) {
    if(m.value) {
      state.throwError("Memory inside .BSS ("+m.varName+") can not have a value!");
    }
  }

  if(stateVars.length === 0)return;

  let currentAddr = 0;
  let lastVar = stateVars[0];
  for(const stateVar of stateVars) {
    const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
    const byteSize = TYPE_SIZE[stateVar.varType] * arraySize;
    let align = stateVar.align || (2 ** TYPE_ALIGNMENT[stateVar.varType]);

    let alignDiff = currentAddr % (align);
    if(alignDiff > 0) {
      state.logInfo(`Info: Alignment gap (${alignDiff} bytes) between ${lastVar.varName} and ${stateVar.varName}`);
    }

    currentAddr += alignDiff + byteSize;
    lastVar = stateVar;
  }
};

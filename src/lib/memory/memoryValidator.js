/**
* @copyright 2024 - Max Bebök
* @license Apache-2.0
*/

import state from "../state.js";
import {TYPE_ALIGNMENT, TYPE_SIZE} from "../dataTypes/dataTypes.js";

/**
 * validates state memory
 * @param {ASTState[]} mem
 * @param {ASTState[]} tempMem
 */
export const validateMemory = (mem, tempMem) => {

  let stateVars = [...mem, ...tempMem]
    .filter(v => !v.extern);

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

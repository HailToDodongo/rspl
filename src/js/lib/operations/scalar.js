/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import state from "../state";
import {isSigned, toHex, toHexSafe} from "../types/types";

function opMove(varRes, varRight)
{
  if(varRight.reg) {
    if(varRes.reg === varRight.reg)return [["## NOP: self-assign!"]];
    return [["move", varRes.reg, varRight.reg]];
  }

  return [["li", varRes.reg, toHexSafe(varRight.value)]];
}

function opAdd(varRes, varLeft, varRight)
{
  let instr = varRight.reg ? "add" : "addi";
  if(!isSigned(varRes.type))instr += "u";

  const valRight = varRight.reg ? varRight.reg : varRight.value;
  return [[instr, varRes.reg, varLeft.reg, valRight]];
}

function opMul(varRes, varLeft, varRight)
{
  state.throwError("Scalar-Multiplication not implemented!");
}

function opShiftLeft(varRes, varLeft, varRight)
{
  return [varRight.reg
    ? ["sllv", varRes.reg, varLeft.reg, varRight.reg]
    : ["sll",  varRes.reg, varLeft.reg, varRight.value],
  ];
}

function opShiftRight(varRes, varLeft, varRight)
{
  let instr = isSigned(varRes.type) ? "sra" : "srl";
  if(varRight.reg)instr += "v";

  const valRight = varRight.reg ? varRight.reg : varRight.value;
  return [[instr, varRes.reg, varLeft.reg, valRight]];
}

function opAnd(varRes, varLeft, varRight)
{
  return [varRight.reg
    ? ["and",  varRes.reg, varLeft.reg, varRight.reg]
    : ["andi", varRes.reg, varLeft.reg, toHexSafe(varRight.value)],
  ];
}

export default {opMove, opAdd, opMul, opShiftLeft, opShiftRight, opAnd};
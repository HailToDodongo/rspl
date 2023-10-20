/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import state from "../state";
import {isSigned, toHex} from "../types/types";

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
    : ["andi", varRes.reg, varLeft.reg, toHex(varRight.value)],
  ];
}

export default {opAdd, opMul, opShiftLeft, opShiftRight, opAnd};
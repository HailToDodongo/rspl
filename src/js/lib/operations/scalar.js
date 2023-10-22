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

function opSub(varRes, varLeft, varRight)
{
  const signed = isSigned(varRes.type);
  if(varRight.reg) {
    return [[signed ? "sub" : "subu", varRes.reg, varLeft.reg, varRight.reg]];
  }
  if(typeof(varRight.value) === "string")state.throwError("Subtraction cannot use labels!");
  return [[signed ? "addi" : "addiu", varRes.reg, varLeft.reg, "-" + varRight.value]];
}

function opMul(varRes, varLeft, varRight) {
  state.throwError("Scalar-Multiplication not implemented!");
}

function opDiv(varRes, varLeft, varRight) {
  state.throwError("Scalar-Division not implemented!");
}

function opShiftLeft(varRes, varLeft, varRight)
{
  if(typeof(varRight.value) === "string")state.throwError("Shift-Left cannot use labels!");
  return [varRight.reg
    ? ["sllv", varRes.reg, varLeft.reg, varRight.reg]
    : ["sll",  varRes.reg, varLeft.reg, varRight.value],
  ];
}

function opShiftRight(varRes, varLeft, varRight)
{
  if(typeof(varRight.value) === "string")state.throwError("Shift-Right cannot use labels!");
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

function opOr(varRes, varLeft, varRight)
{
  return [varRight.reg
    ? ["or",  varRes.reg, varLeft.reg, varRight.reg]
    : ["ori", varRes.reg, varLeft.reg, toHexSafe(varRight.value)],
  ];
}

function opXOR(varRes, varLeft, varRight)
{
  return [varRight.reg
    ? ["xor",  varRes.reg, varLeft.reg, varRight.reg]
    : ["xori", varRes.reg, varLeft.reg, toHexSafe(varRight.value)],
  ];
}

function opBitFlip(varRes, varRight)
{
  if(!varRight.reg)state.throwError("Bitflip is only supported for variables!");
  return [["not", varRes.reg, varRight.reg]];
}

export default {opMove, opAdd, opSub, opMul, opDiv, opShiftLeft, opShiftRight, opAnd, opOr, opXOR, opBitFlip};
/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import state from "../state";
import {isSigned, toHex, toHexSafe} from "../types/types";
import {normReg} from "../syntax/registers";

function opMove(varRes, varRight)
{
  if(varRight.reg) {
    if(varRes.reg === varRight.reg)return [["## NOP: self-assign!"]];
    return [["move", varRes.reg, varRight.reg]];
  }

  return [["li", varRes.reg, toHexSafe(varRight.value)]];
}

function opLoad(varRes, varLoc, varOffset)
{
  const offsetStr = varOffset.type === "num" ? varOffset.value : `%lo(${varOffset.name})`;
  if(varLoc.reg) {
    return [["lw", varRes.reg, `${offsetStr}(${normReg(varLoc.reg)})`]];
  }

  if(varOffset.type !== "num")state.throwError("Load args cannot both be consts!");
  return [["lw", varRes.reg, `%lo(${varLoc.name} + ${offsetStr})`]];
}


function opBranch(compare, regTest, labelElse)
{
  if(compare.right.type === "var") {
    regTest = state.getRequiredVar(compare.right.value, "compare").reg;
  }

  const isConst = compare.right.type === "num";
  const regBase = state.getRequiredVar(compare.left.value, "left").reg;
  const regOrValTest = isConst ? compare.right.value : regTest;

  // @TODO: handle optimized compares against zero
  // (BLTZ, BGEZ, BLTZAL, BGEZAL, BEQ, BNE, BLEZ, BGTZ)

  const lessThanIn = "slt" +
    (isConst ? "i" : "") +
    (isSigned(compare.left.type) ? "" : "u");

  // Note: the "true" case is expected to follow the branch.
  // So we only jump if it's false, therefore the "beq"/"bne" are inverted.
  switch (compare.op)
  {
    case "==": return [
      isConst ? ["lui", regTest, regOrValTest] : [],
      ["bne", regBase, regTest, labelElse+"f"], ["nop"],
    ];
    case "!=": return [
      isConst ? ["lui", regTest, regOrValTest] : [],
      ["beq", regBase, regTest, labelElse+"f"], ["nop"],
    ];
    case "<": return [
      [lessThanIn, regTest, regBase, regOrValTest],
      ["beq", regTest, "$zero", labelElse+"f"], ["nop"],
    ];
    case ">": return [
      [lessThanIn, regTest, regOrValTest, regBase],
      ["beq", regTest, "$zero", labelElse+"f"], ["nop"],
    ];
    case "<=": return [
      [lessThanIn, regTest, regOrValTest, regBase],
      ["bne", regTest, "$zero", labelElse+"f"], ["nop"],
    ];
    case ">=": return [
      [lessThanIn, regTest, regOrValTest, regBase],
      ["bne", regTest, "$zero", labelElse+"f"], ["nop"],
    ];

    default:
      return state.throwError("Unknown comparison operator: " + compare.op, compare);
  }
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

export default {opMove, opLoad, opBranch, opAdd, opSub, opMul, opDiv, opShiftLeft, opShiftRight, opAnd, opOr, opXOR, opBitFlip};
/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import state from "../state";
import {isSigned, toHex, toHexSafe} from "../types/types";
import {normReg} from "../syntax/registers";
import {asm, asmComment, asmNOP} from "../intsructions/asmWriter.js";

function opMove(varRes, varRight)
{
  if(varRight.reg) {
    if(varRes.reg === varRight.reg)return [asmComment("NOP: self-assign!")];
    return [asm("move", [varRes.reg, varRight.reg])];
  }

  return [asm("li", [varRes.reg, toHexSafe(varRight.value)])];
}

function opLoad(varRes, varLoc, varOffset)
{
  const offsetStr = varOffset.type === "num" ? varOffset.value : `%lo(${varOffset.name})`;
  if(varLoc.reg) {
    return [asm("lw", [varRes.reg, `${offsetStr}(${normReg(varLoc.reg)})`])];
  }

  if(varOffset.type !== "num")state.throwError("Load args cannot both be consts!");
  return [asm("lw", [varRes.reg, `%lo(${varLoc.name} + ${offsetStr})`])];
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
      isConst ? asm("lui", [regTest, regOrValTest]) : null,
      asm("bne", [regBase, regTest, labelElse+"f"]), asmNOP(),
    ];
    case "!=": return [
      isConst ? asm("lui", [regTest, regOrValTest]) : null,
      asm("beq", [regBase, regTest, labelElse+"f"]), asmNOP()
    ];
    case "<": return [
      asm(lessThanIn, [regTest, regBase, regOrValTest]),
      asm("beq", [regTest, "$zero", labelElse+"f"]), asmNOP(),
    ];
    case ">": return [
      asm(lessThanIn, [regTest, regOrValTest, regBase]),
      asm("beq", [regTest, "$zero", labelElse+"f"]), asmNOP(),
    ];
    case "<=": return [
      asm(lessThanIn, [regTest, regOrValTest, regBase]),
      asm("bne", [regTest, "$zero", labelElse+"f"]), asmNOP(),
    ];
    case ">=": return [
      asm(lessThanIn, [regTest, regOrValTest, regBase]),
      asm("bne", [regTest, "$zero", labelElse+"f"]), asmNOP(),
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
  return [asm(instr, [varRes.reg, varLeft.reg, valRight])];
}

function opSub(varRes, varLeft, varRight)
{
  const signed = isSigned(varRes.type);
  if(varRight.reg) {
    return [asm(signed ? "sub" : "subu", [varRes.reg, varLeft.reg, varRight.reg])];
  }
  if(typeof(varRight.value) === "string")state.throwError("Subtraction cannot use labels!");
  return [asm(signed ? "addi" : "addiu", [varRes.reg, varLeft.reg, "-" + varRight.value])];
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
    ? asm("sllv", [varRes.reg, varLeft.reg, varRight.reg])
    : asm("sll",  [varRes.reg, varLeft.reg, varRight.value])
  ];
}

function opShiftRight(varRes, varLeft, varRight)
{
  if(typeof(varRight.value) === "string")state.throwError("Shift-Right cannot use labels!");
  let instr = isSigned(varRes.type) ? "sra" : "srl";
  if(varRight.reg)instr += "v";

  const valRight = varRight.reg ? varRight.reg : varRight.value;
  return [asm(instr, [varRes.reg, varLeft.reg, valRight])];
}

function opAnd(varRes, varLeft, varRight)
{
  return [varRight.reg
    ? asm("and",  [varRes.reg, varLeft.reg, varRight.reg])
    : asm("andi", [varRes.reg, varLeft.reg, toHexSafe(varRight.value)])
  ];
}

function opOr(varRes, varLeft, varRight)
{
  return [varRight.reg
    ? asm("or",  [varRes.reg, varLeft.reg, varRight.reg])
    : asm("ori", [varRes.reg, varLeft.reg, toHexSafe(varRight.value)])
  ];
}

function opXOR(varRes, varLeft, varRight)
{
  return [varRight.reg
    ? asm("xor",  [varRes.reg, varLeft.reg, varRight.reg])
    : asm("xori", [varRes.reg, varLeft.reg, toHexSafe(varRight.value)])
  ];
}

function opBitFlip(varRes, varRight)
{
  if(!varRight.reg)state.throwError("Bitflip is only supported for variables!");
  return [asm("not", [varRes.reg, varRight.reg])];
}

export default {opMove, opLoad, opBranch, opAdd, opSub, opMul, opDiv, opShiftLeft, opShiftRight, opAnd, opOr, opXOR, opBitFlip};
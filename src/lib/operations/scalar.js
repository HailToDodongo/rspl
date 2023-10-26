/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import state from "../state";
import {isSigned, toHex, toHexSafe} from "../types/types";
import {normReg, REG} from "../syntax/registers";
import {asm, asmComment, asmNOP} from "../intsructions/asmWriter.js";

function u32InS16Range(valueU32) {
  return valueU32 <= 0x7FFF || valueU32 >= 0xFFFF8000;
}

/**
 * Loads a 32bit int into a register with as few instructions as possible.
 * @param regDst target register
 * @param value value to load (can be a string for labels
 * @returns {{args: *, op: *, type: number}[]}
 */
function loadImmediate(regDst, value)
{
  if(typeof(value) === "string") { // Labels are always <= 16bit
    return [asm("ori", [regDst, REG.ZERO, value])];
  }
  const valueU32 = parseInt(value) >>> 0; // (s32 -> u32)

  if(valueU32 === 0) { // don't know, GCC prefers a move on zero
    return [asm("or", [regDst, REG.ZERO, REG.ZERO])];
  }

  // small positive/negative values can be added onto zero
  // this also helps for "big" unsigned-numbers (e.g. "0xFFFF8123") by treating them as signed
  if(u32InS16Range(valueU32)) {
    return [asm("addiu", [regDst, REG.ZERO, valueU32 >> 0])]; // (shift-right forces a sign in JS)
  }

  if(valueU32 <= 0xFFFF) { // fits into 16bit -> use OR with zero
    return [asm("ori", [regDst, REG.ZERO, toHex(valueU32)])];
  }

  if((valueU32 & 0xFFFF) === 0) { // >16bit, but lower 16bit are zero -> load only upper 16bit
    return [asm("lui", [regDst, toHex(valueU32 >>> 16)])];
  }

  return [ // too large, resort to using two instructions
    asm("lui", [regDst, toHex(valueU32 >>> 16)]),
    asm("ori", [regDst, regDst, toHex(valueU32 & 0xFFFF)])
  ];
}

function opMove(varRes, varRight)
{
  if(varRight.reg) {
    if(varRes.reg === varRight.reg)return [asmComment("NOP: self-assign!")];
    return [asm("or", [varRes.reg, REG.ZERO, varRight.reg])];
  }
  return loadImmediate(varRes.reg, varRight.value);
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
  const varBase = state.getRequiredVar(compare.left.value, "left");
  const regBase = varBase.reg;
  const regOrValTest = isConst ? compare.right.value : regTest;
console.log("opBranch", {compare, regTest, labelElse, isConst, varBase, regBase, regOrValTest});
  // @TODO: handle optimized compares against zero
  // (BLTZ, BGEZ, BLTZAL, BGEZAL, BEQ, BNE, BLEZ, BGTZ)

  // @TODO: add load const into reg function (signed unsigned, > 16bit)
  // @TODO: compare op for <, >, <=, >=

  const lessThanIn = "slt" +
    (isConst ? "i" : "") +
    (isSigned(varBase.type) ? "" : "u");

  // Note: the "true" case is expected to follow the branch.
  // So we only jump if it's false, therefore the "beq"/"bne" are inverted.
  switch (compare.op)
  {
    case "==": return [
      asm("bne", [regBase, regTest, labelElse+"f"]),
      isConst ? asm("lui", [regTest, regOrValTest]) : asmNOP(),
    ];
    case "!=": return [
      asm("beq", [regBase, regTest, labelElse+"f"]),
      isConst ? asm("lui", [regTest, regOrValTest]) : asmNOP(),
    ];
    case "<": return [
      asm("beq", [regTest, REG.ZERO, labelElse+"f"]),
      asm(lessThanIn, [regTest, regBase, regOrValTest]),
    ];
    case ">": return [
      asm("beq", [regTest, REG.ZERO, labelElse+"f"]),
      asm(lessThanIn, [regTest, regOrValTest, regBase]),
    ];
    case "<=": return [
      asm("bne", [regTest, REG.ZERO, labelElse+"f"]),
      asm(lessThanIn, [regTest, regBase, regOrValTest]),
    ];
    case ">=": return [
      asm("bne", [regTest, REG.ZERO, labelElse+"f"]),
      asm(lessThanIn, [regTest, regBase, regOrValTest]),
    ];

    default:
      return state.throwError("Unknown comparison operator: " + compare.op, compare);
  }
}

function opAdd(varRes, varLeft, varRight)
{
  if(varRight.reg) {
    return [asm("addu", [varRes.reg, varLeft.reg, varRight.reg])];
  }

  if(typeof varRight.value === "string") {
    return [asm("addiu", [varRes.reg, varLeft.reg, varRight.value])];
  }

  const valU32 = parseInt(varRight.value) >>> 0;
  if(u32InS16Range(valU32)) {
    return [asm("addiu", [varRes.reg, varLeft.reg, valU32 & 0xFFFF])];
  }
  return [
    ...loadImmediate(REG.AT, valU32),
    asm("addu", [varRes.reg, varLeft.reg, REG.AT])
  ];
}

function opSub(varRes, varLeft, varRight)
{
  if(varRight.reg) {
    return [asm("subu", [varRes.reg, varLeft.reg, varRight.reg])];
  }
  if(typeof(varRight.value) === "string")state.throwError("Subtraction cannot use labels!");
  return opAdd(varRes, varLeft, {reg: varRight.reg, value: -varRight.value});
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
// @TODO: safe range
  return [varRight.reg
    ? asm("and",  [varRes.reg, varLeft.reg, varRight.reg])
    : asm("andi", [varRes.reg, varLeft.reg, toHexSafe(varRight.value)])
  ];
}

function opOr(varRes, varLeft, varRight)
{
// @TODO: safe range
  return [varRight.reg
    ? asm("or",  [varRes.reg, varLeft.reg, varRight.reg])
    : asm("ori", [varRes.reg, varLeft.reg, toHexSafe(varRight.value)])
  ];
}

function opXOR(varRes, varLeft, varRight)
{
// @TODO: safe range
  return [varRight.reg
    ? asm("xor",  [varRes.reg, varLeft.reg, varRight.reg])
    : asm("xori", [varRes.reg, varLeft.reg, toHexSafe(varRight.value)])
  ];
}

function opBitFlip(varRes, varRight)
{
  if(!varRight.reg)state.throwError("Bitflip is only supported for variables!");
  return [asm("nor", [varRes.reg, REG.ZERO, varRight.reg])];
}

export default {opMove, opLoad, opBranch, opAdd, opSub, opMul, opDiv, opShiftLeft, opShiftRight, opAnd, opOr, opXOR, opBitFlip};
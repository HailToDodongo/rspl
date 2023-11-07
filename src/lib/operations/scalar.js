/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import state from "../state";
import {isSigned, toHex, u32InS16Range, u32InU16Range} from "../types/types";
import {fractReg, intReg, REG} from "../syntax/registers";
import {asm, asmComment} from "../intsructions/asmWriter.js";
import {SWIZZLE_MAP} from "../syntax/swizzle.js";

const MUL_TO_SHIFT = {}
for(let i = 0; i < 32; i++)MUL_TO_SHIFT[Math.pow(2, i)] = i;

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
    if(varRight.swizzle && !varRight.type.startsWith("vec")) {
      state.throwError("Swizzling not allowed for scalar operations!");
    }
    if(varRes.reg === varRight.reg)return [asmComment("NOP: self-assign!")];
    if(varRight.swizzle) {
      const swizzle =  SWIZZLE_MAP[varRight.swizzle];
      if(varRight.type === "vec16") {
        return [asm("mfc2", [varRes.reg, intReg(varRight) + swizzle])];
      }
      return [
        asm("mfc2", [varRes.reg, fractReg(varRight) + swizzle]),
        asm("andi", [varRes.reg, varRes.reg, 0xFFFF]),
        asm("mfc2", [REG.AT, intReg(varRight) + swizzle]),
        asm("sll", [REG.AT, REG.AT, 16]),
        asm("or", [varRes.reg, varRes.reg, REG.AT])
      ];
    }

    return [asm("or", [varRes.reg, REG.ZERO, varRight.reg])];
  }
  return loadImmediate(varRes.reg, varRight.value);
}

function opLoad(varRes, varLoc, varOffset)
{
  const offsetStr = varOffset.type === "num" ? varOffset.value : `%lo(${varOffset.name})`;
  const loadOp = {
    "u8":  "lb", "s8":  "lb",
    "u16": "lh", "s16": "lh",
    "u32": "lw", "s32": "lw",
  }[varRes.type];

  if(varLoc.reg) {
    return [asm(loadOp, [varRes.reg, `${offsetStr}(${varLoc.reg})`])];
  }

  if(varOffset.type !== "num")state.throwError("Load args cannot both be consts!");
  return [asm(loadOp, [varRes.reg, `%lo(${varLoc.name} + ${offsetStr})`])];
}

function opStore(varRes, varOffsets)
{
  const varLoc = state.getRequiredVarOrMem(varOffsets[0].value, "base");

  const offsets = varOffsets.slice(1);
  if(!varLoc.reg) {
    offsets.push({type: "const", value: varLoc.name});
  }

  const offsetStr = offsets
    .map(v => v.type === "num" ? v.value : `%lo(${v.value})`)
    .join(" + ");

  const baseReg = varLoc.reg || REG.ZERO;

  return [asm("sw", [varRes.reg, `${offsetStr}(${baseReg})`])];

  /*switch (varRes.type) {
    case "u8":
    case "s8": return [asm("sb", [varSrc.reg, "0x0", varRes.reg])];
    case "u16":
    case "s16": return [asm("sh", [varSrc.reg, "0x0", varRes.reg])];
    case "u32":
    case "s32": return [asm("sw", [varSrc.reg, "0x0", varRes.reg])];
    default: state.throwError("Unknown type: " + varRes.type, varRes);
  }*/
}

function opRegOrImmediate(opReg, opImm, rangeCheckFunc, varRes, varLeft, varRight)
{
  if(varRight.reg) {
    return [asm(opReg, [varRes.reg, varLeft.reg, varRight.reg])];
  }

  if(typeof varRight.value === "string") {
    return [asm(opImm, [varRes.reg, varLeft.reg, varRight.value])];
  }

  const valU32 = parseInt(varRight.value) >>> 0;
  if(rangeCheckFunc(valU32)) {
    return [asm(opImm, [varRes.reg, varLeft.reg, valU32 & 0xFFFF])];
  }
  return [
    ...loadImmediate(REG.AT, valU32),
    asm(opReg, [varRes.reg, varLeft.reg, REG.AT])
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

function opAdd(varRes, varLeft, varRight) {
  return opRegOrImmediate("addu", "addiu", u32InS16Range, varRes, varLeft, varRight);
}

function opShiftLeft(varRes, varLeft, varRight)
{
  if(typeof(varRight.value) === "string")state.throwError("Shift-Left cannot use labels!");
  if(varRight.value < 0 || varRight.value > 31) {
    state.throwError("Shift-Left value must be in range 0<x<32!");
  }

  return [varRight.reg
    ? asm("sllv", [varRes.reg, varLeft.reg, varRight.reg])
    : asm("sll",  [varRes.reg, varLeft.reg, varRight.value])
  ];
}

function opShiftRight(varRes, varLeft, varRight)
{
  if(typeof(varRight.value) === "string")state.throwError("Shift-Right cannot use labels!");
  if(varRight.value < 0 || varRight.value > 31) {
    state.throwError("Shift-Right value must be in range 0<x<32!");
  }

  let instr = isSigned(varRes.type) ? "sra" : "srl";
  if(varRight.reg)instr += "v";

  const valRight = varRight.reg ? varRight.reg : varRight.value;
  return [asm(instr, [varRes.reg, varLeft.reg, valRight])];
}

function opAnd(varRes, varLeft, varRight) {
  return opRegOrImmediate("and", "andi", u32InU16Range, varRes, varLeft, varRight);
}

function opOr(varRes, varLeft, varRight) {
  return opRegOrImmediate("or", "ori", u32InU16Range, varRes, varLeft, varRight);
}

function opXOR(varRes, varLeft, varRight) {
  return opRegOrImmediate("xor", "xori", u32InU16Range, varRes, varLeft, varRight);
}

function opBitFlip(varRes, varRight)
{
  if(!varRight.reg)state.throwError("Bitflip is only supported for variables!");
  return [asm("nor", [varRes.reg, REG.ZERO, varRight.reg])];
}

function opMul(varRes, varLeft, varRight) {
  const shiftVal = MUL_TO_SHIFT[varRight.value || 0];
  if(varRight.reg || shiftVal === undefined) {
    state.throwError("Scalar-Multiplication only allowed with a power-of-two constant on the right side!\nFor example 'a = b * 4;' or 'a *= 8;' is allowed.", [varRes, varLeft, varRight]);
  }
  if(varRight.value === 1) {
    state.throwError("Scalar-Multiplication with 1 is a NOP!", [varRes, varLeft, varRight]);
  }
  return opShiftLeft(varRes, varLeft, {type: 'u32', value: shiftVal});
}
function opDiv(varRes, varLeft, varRight) {
  const shiftVal = MUL_TO_SHIFT[varRight.value || 0];
  if(varRight.reg || shiftVal === undefined) {
    state.throwError("Scalar-Division only allowed with a power-of-two constant on the right side!\nFor example 'a = b / 4;' or 'a /= 8;' is allowed.", [varRes, varLeft, varRight]);
  }
  if(varRight.value === 1) {
    state.throwError("Scalar-Division by 1 is a NOP!", [varRes, varLeft, varRight]);
  }
  return opShiftRight(varRes, varLeft, {type: 'u32', value: shiftVal});
}
export default {
  opMove, opLoad, opStore, opAdd, opSub, opMul, opDiv, opShiftLeft, opShiftRight, opAnd, opOr, opXOR, opBitFlip,
  loadImmediate
};
/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import state from "../state";
import {isSigned, toHex, u32InS16Range, u32InU16Range} from "../dataTypes/dataTypes.js";
import {fractReg, intReg, REG} from "../syntax/registers";
import {asm, asmComment} from "../intsructions/asmWriter.js";
import {SWIZZLE_MAP} from "../syntax/swizzle.js";

const MUL_TO_SHIFT = {}
for(let i = 0; i < 32; i++)MUL_TO_SHIFT[Math.pow(2, i)] = i;

/**
 * Loads a 32bit integer into a register with as few instructions as possible.
 * @param {string} regDst target register
 * @param {number|string} value value to load (can be a string for labels)
 * @returns {ASM[]}
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

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opMove(varRes, varRight)
{
  if(varRes.swizzle)state.throwError("Swizzling not allowed on scalar variables!");

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

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLoc
 * @param {ASTFuncArg} varOffset
 * @returns {ASM[]}
 */
function opLoad(varRes, varLoc, varOffset)
{
  const offsetStr = varOffset.type === "num" ? varOffset.value : `%lo(${varOffset.name})`;
  const loadOp = {
    u8:  "lbu", s8:  "lb",
    u16: "lhu", s16: "lh",
    u32: "lw",  s32: "lw", // no "lwu", 32bit CPU
  }[varRes.type];

  if(varLoc.reg) {
    return [asm(loadOp, [varRes.reg, `${offsetStr}(${varLoc.reg})`])];
  }

  if(varOffset.type !== "num")state.throwError("Load args cannot both be consts!");
  return [asm(loadOp, [varRes.reg, `%lo(${varLoc.name} + ${offsetStr})`])];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg[]} varOffsets
 * @returns {ASM[]}
 */
function opStore(varRes, varOffsets)
{
  const varLoc = state.getRequiredVarOrMem(varOffsets[0].value, "base");

  const offsets = varOffsets.slice(1);
  if(!varLoc.reg) {
    offsets.push({type: "var", value: varLoc.name});
  }

  const offsetStr = offsets
    .map(v => v.type === "num" ? v.value : `%lo(${v.value})`)
    .join(" + ");

  const baseReg = varLoc.reg || REG.ZERO;
  const op = {
     u8: "sb",  s8: "sb",
    u16: "sh", s16: "sh",
    u32: "sw", s32: "sw",
  }[varRes.type];

  return [asm(op, [varRes.reg, `${offsetStr}(${baseReg})`])];
}

/**
 * Generic Operation with either a register or an immediate value.
 * @param {string} opReg opcode for the register version
 * @param {string} opImm opcode for the immediate version
 * @param {(number) => boolean} rangeCheckFunc checks if the value is in range for the immediate version
 * @param {ASTFuncArg} varRes result variable
 * @param {ASTFuncArg} varLeft left operand
 * @param {ASTFuncArg} varRight right operand
 * @return {ASM[]|(ASM|ASM)[]}
 */
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

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opSub(varRes, varLeft, varRight)
{
  if(varRight.reg) {
    return [asm("subu", [varRes.reg, varLeft.reg, varRight.reg])];
  }
  if(typeof(varRight.value) === "string")state.throwError("Subtraction cannot use labels!");
  return opAdd(varRes, varLeft, {type: 's32', reg: varRight.reg, value: -varRight.value});
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opAdd(varRes, varLeft, varRight) {
  return opRegOrImmediate("addu", "addiu", u32InS16Range, varRes, varLeft, varRight);
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
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

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
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

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opAnd(varRes, varLeft, varRight) {
  return opRegOrImmediate("and", "andi", u32InU16Range, varRes, varLeft, varRight);
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opOr(varRes, varLeft, varRight) {
  return opRegOrImmediate("or", "ori", u32InU16Range, varRes, varLeft, varRight);
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opXOR(varRes, varLeft, varRight) {
  return opRegOrImmediate("xor", "xori", u32InU16Range, varRes, varLeft, varRight);
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opBitFlip(varRes, varRight)
{
  if(!varRight.reg)state.throwError("Bitflip is only supported for variables!");
  return [asm("nor", [varRes.reg, REG.ZERO, varRight.reg])];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
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

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
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
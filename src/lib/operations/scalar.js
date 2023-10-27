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

function u32InU16Range(valueU32) {
  return valueU32 <= 0xFFFF;
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


function opBranch(compare, labelElse)
{
  // Note: the "true" case is expected to follow the branch.
  // So we only jump if it's false, therefore all checks are inverted.

  compare = structuredClone(compare); // gets modified

  let isImmediate = compare.right.type === "num";
  let regTestRes = isImmediate ? REG.AT
    : state.getRequiredVar(compare.right.value, "compare").reg;

  // use register for zero-checks (avoids potential imm. load instructions)
  if(isImmediate && compare.right.value === 0) {
    isImmediate = false;
    regTestRes = REG.ZERO;
  }

  let {reg: regLeft, type: baseType} = state.getRequiredVar(compare.left.value, "left");

  // Easy case, just compare
  if(compare.op === "==" || compare.op === "!=")
  {
    const opBranch = compare.op === "==" ? "bne" : "beq";
    return [
      ...(isImmediate ? loadImmediate(REG.AT, compare.right.value) : []),
      asm(opBranch, [regLeft, regTestRes, labelElse+"f"]),
      asmNOP(),
    ];
  }

  const opsLoad = [];
  let regOrValRight = isImmediate ? compare.right.value : regTestRes;

  // Both ">" and "<=" are causing the biggest issues when inverted, so map them to the other two
  if(compare.op === ">" || compare.op === "<=") {
    if(isImmediate) {
      // add one to the immediate to add/remove the "=" part of the comparison.
      // Ignore overflows here (e.g. "x > 0xFFFFFFFF" would be stupid anyway)
      regOrValRight = (regOrValRight+1) >>> 0;
      compare.op = compare.op === ">" ? ">=" : "<";
    } else {
      compare.op = compare.op === ">" ? "<" : ">="; // invert comparison ...
      [regLeft, regOrValRight] = [regOrValRight, regLeft]; //... and swap registers
    }
  }

  // All values from now on need signedness checks, first check if it can still be an immediate
  if(isImmediate && !u32InS16Range(regOrValRight >>> 0)) {
    // ...if it doesn't we load it and switch back to a register check
    opsLoad.push(...loadImmediate(REG.AT, regOrValRight));
    isImmediate = false;
    regOrValRight = REG.AT;
  }

  const signed = isSigned(baseType); // Note: both the signed/unsigned 'slt' have a sign-extended immediate
  const opLessThan = "slt" + (isImmediate ? "i" : "") + (signed ? "" : "u");

  if(compare.op === "<" || compare.op === ">=")
  {
    const opBranch = compare.op === "<" ? "beq" : "bne";
    return [
      ...opsLoad,
      asm(opLessThan, [REG.AT, regLeft, regOrValRight]),
      asm(opBranch, [REG.AT, REG.ZERO, labelElse+"f"]), // jump if "<" fails (aka ">=")
      asmNOP(),
    ];
  }

  return state.throwError("Unknown comparison operator: " + compare.op, compare);
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

function opMul() { state.throwError("Scalar-Multiplication not implemented!"); }
function opDiv() { state.throwError("Scalar-Division not implemented!"); }

export default {opMove, opLoad, opBranch, opAdd, opSub, opMul, opDiv, opShiftLeft, opShiftRight, opAnd, opOr, opXOR, opBitFlip};
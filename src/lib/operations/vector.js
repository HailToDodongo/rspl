/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import {isVecReg, nextVecReg, normReg} from "../syntax/registers";
import state from "../state";
import {isScalarSwizzle, SWIZZLE_MAP, SWIZZLE_SCALAR_IDX} from "../syntax/swizzle";
import {toHex} from "../types/types";
import {asm} from "../intsructions/asmWriter.js";

function opMove(varRes, varRight)
{
  if(!varRight.reg) {
    return state.throwError("Constant cannot be assigned to a vector!");
  }
  if(!varRes.swizzle || !varRight.swizzle) {
    return state.throwError("Vector assignment must be swizzled (single-lane only)!");
  }
  if(!isScalarSwizzle(varRes.swizzle) || !isScalarSwizzle(varRight.swizzle)) {
    return state.throwError("Vector swizzle must be single-lane! (.x to .W)");
  }

  const swizzleRes   = SWIZZLE_MAP[varRes.swizzle || ""];
  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];

  return [asm("vmov", [varRes.reg + swizzleRes, varRight.reg + swizzleRight])];
}

function opLoad(varRes, varLoc, varOffset, swizzle)
{
  if(swizzle && swizzle !== "xyzwxyzw") {
    state.throwError("Builtin load() only support '.xyzwxyzw' swizzle!", varRes);
  }
  if(!varLoc.reg)state.throwError("Load base-address must be a variable!");
  if(isVecReg(varLoc.reg))state.throwError("Load base-address must be a scalar register!", varRes);
  if(varOffset.type !== "num")state.throwError("Load offset must be a numerical-constant!");

  let destOffset = varRes.swizzle ? SWIZZLE_SCALAR_IDX[varRes.swizzle] : 0;
  if(destOffset === undefined) {
    state.throwError("Unsupported destination swizzle (must be scalar .x to .W)!", varRes);
  }
  destOffset *= 2; // convert to bytes (?)

  const loadInstr = swizzle ? "ldv" : "lqv";

  const res = [];
  res.push(           asm(loadInstr, [varRes.reg, toHex(destOffset), varOffset.value, varLoc.reg]));
  if(swizzle)res.push(asm(loadInstr, [varRes.reg, toHex(destOffset+8), varOffset.value, varLoc.reg]));

  if(varRes.type === "vec32") {
    res.push(           asm(loadInstr, [nextVecReg(varRes.reg), toHex(destOffset), varOffset.value + " + 0x10", varLoc.reg]));
    if(swizzle)res.push(asm(loadInstr, [nextVecReg(varRes.reg), toHex(destOffset+8), varOffset.value + " + 0x10", varLoc.reg]));
  }
  return res;
}

function opAdd(varRes, varLeft, varRight, clearAccum)
{
  const res = [];
  state.throwError("Vector-Addition not implemented!");
  return res;
}

function opMul(varRes, varLeft, varRight, clearAccum)
{
  if(!varRight.reg) {
    state.throwError("Multiplication cannot be done with a constant!");
  }
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Multiplication only allows swizzle on the right side!");
  }

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  const firstIn = clearAccum ? "vmudl" : "vmadl";

  return [
    asm(firstIn,  [nextVecReg(varRes.reg), nextVecReg(varLeft.reg), nextVecReg(varRight.reg) + swizzleRight]),
    asm("vmadm",  [nextVecReg(varRes.reg),            varLeft.reg,  nextVecReg(varRight.reg) + swizzleRight]),
    asm("vmadn",  [nextVecReg(varRes.reg), nextVecReg(varLeft.reg), varRight.reg             + swizzleRight]),
    asm("vmadh",  [           varRes.reg,             varLeft.reg,  varRight.reg             + swizzleRight]),
  ];
}

export default {opMove, opLoad, opMul};
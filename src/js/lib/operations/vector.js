/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import {nextVecReg} from "../syntax/registers";
import state from "../state";
import {isScalarSwizzle, SWIZZLE_MAP} from "../syntax/swizzle";
import {toHex} from "../types/types";

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

  return [["vmov", varRes.reg + swizzleRes, varRight.reg + swizzleRight]];
}

function opAdd(varRes, varLeft, varRight, clearAccum)
{
  const res = [];
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
    [firstIn,  nextVecReg(varRes.reg), nextVecReg(varLeft.reg), nextVecReg(varRight.reg) + swizzleRight],
    ["vmadm",  nextVecReg(varRes.reg),            varLeft.reg,  nextVecReg(varRight.reg) + swizzleRight],
    ["vmadn",  nextVecReg(varRes.reg), nextVecReg(varLeft.reg), varRight.reg             + swizzleRight],
    ["vmadh",             varRes.reg,             varLeft.reg,  varRight.reg             + swizzleRight],
    [],
  ];
}

export default {opMove, opMul};
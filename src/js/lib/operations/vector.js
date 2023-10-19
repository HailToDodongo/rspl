/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import {nextVecReg} from "../syntax/registers";
import state from "../state";
import {SWIZZLE_MAP} from "../syntax/swizzle";

function opAdd(varRes, varLeft, varRight, clearAccum)
{
  const res = [];
  return res;
}

function opMul(varRes, varLeft, varRight, clearAccum)
{
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Multiplication only allows swizzle on the right side!");
  }

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  const firstIn = clearAccum ? "vmudl" : "vmadl";

  return [
    [],
    [firstIn,  nextVecReg(varRes.reg), nextVecReg(varLeft.reg), nextVecReg(varRight.reg), swizzleRight],
    ["vmadm",  nextVecReg(varRes.reg),            varLeft.reg,  nextVecReg(varRight.reg), swizzleRight],
    ["vmadn",  nextVecReg(varRes.reg), nextVecReg(varLeft.reg), varRight.reg,             swizzleRight],
    ["vmadh",             varRes.reg,             varLeft.reg,  varRight.reg,             swizzleRight],
  ];
}

export default {opAdd, opMul};
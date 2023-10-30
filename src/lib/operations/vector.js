/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import {fractReg, isVecReg, nextReg, nextVecReg, REG} from "../syntax/registers";
import state from "../state";
import {isScalarSwizzle, SWIZZLE_MAP, SWIZZLE_SCALAR_IDX} from "../syntax/swizzle";
import {f32ToFP32, toHex} from "../types/types";
import {asm} from "../intsructions/asmWriter.js";
import opsScalar from "./scalar";

function opMove(varRes, varRight)
{
  if(!varRight.reg && !varRes.swizzle) {
    if(varRight.value === 0) {
      return [asm("vxor", [varRes.reg, varRes.reg, varRes.reg])];
    }
    return state.throwError("Can only assign the scalar '0' to a non-swizzled vector!\n"
      + "If you want to load a single element, use 'foo.x = "+varRight.value+";' instead"
    );
  }

  const isConst = !varRight.reg;

  if(!varRes.swizzle || (!isConst && !varRight.swizzle)) {
    return state.throwError("Vector to vector assignment must be swizzled (single-lane only)!");
  }
  if(!isScalarSwizzle(varRes.swizzle) || (!isConst && !isScalarSwizzle(varRight.swizzle))) {
    return state.throwError("Vector swizzle must be single-lane! (.x to .W)");
  }

  const swizzleRes = SWIZZLE_MAP[varRes.swizzle || ""];

  if(isConst) {
    const valueFP32 = f32ToFP32(varRight.value);
    const valInt = ((valueFP32 >>> 16) & 0xFFFF);
    const valFract = (valueFP32 & 0xFFFF);
    const regDst = [varRes.reg, nextVecReg(varRes.reg)];

    const opLoadInt = [
      ...(valInt === 0 ? [] : opsScalar.loadImmediate(REG.AT, valInt)),
      asm("mtc2", [valInt === 0 ? REG.ZERO : REG.AT, regDst[0] + swizzleRes])
    ];
    const opLoadFract = [
      ...(valFract === 0 ? [] : opsScalar.loadImmediate(REG.AT, valFract)),
      asm("mtc2", [valFract === 0 ? REG.ZERO : REG.AT, regDst[1] + swizzleRes]),
    ];

    return varRes.type === "vec32" ? [...opLoadInt, ...opLoadFract] : opLoadInt;
  }

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
  if(!varRight.reg) {
    state.throwError("Addition cannot be done with a constant!");
  }
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Addition only allows swizzle on the right side!");
  }

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  return (varRes.type === "vec32")
    ? [
      asm("vaddc", [nextReg(varRes.reg), fractReg(varLeft), fractReg(varRight) + swizzleRight]),
      asm("vadd",  [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
    ] : [
      asm("vaddc", [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
    ];
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
    asm(firstIn,  [REG.VTEMP,              fractReg(varLeft), fractReg(varRight) + swizzleRight]),
    asm("vmadm",  [REG.VTEMP,                    varLeft.reg, fractReg(varRight) + swizzleRight]),
    asm("vmadn",  [nextVecReg(varRes.reg), fractReg(varLeft),       varRight.reg + swizzleRight]),
    asm("vmadh",  [           varRes.reg,        varLeft.reg,       varRight.reg + swizzleRight]),
  ];
}

export default {opMove, opLoad, opAdd, opMul};
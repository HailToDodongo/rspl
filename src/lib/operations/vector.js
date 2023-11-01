/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import {fractReg, intReg, isVecReg, nextReg, nextVecReg, REG} from "../syntax/registers";
import state from "../state";
import {isScalarSwizzle, POW2_SWIZZLE_VAR, SWIZZLE_MAP, SWIZZLE_SCALAR_IDX} from "../syntax/swizzle";
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
  const isScalar = isConst || !varRight.type.startsWith("vec");

  if(!varRes.swizzle || (!isScalar && !varRight.swizzle)) {
    return state.throwError("Vector to vector assignment must be swizzled (single-lane only)!");
  }
  if(!isScalarSwizzle(varRes.swizzle) || (!isScalar && !isScalarSwizzle(varRight.swizzle))) {
    return state.throwError("Vector swizzle must be single-lane! (.x to .W)");
  }

  const swizzleRes = SWIZZLE_MAP[varRes.swizzle || ""];
  const regDst = [varRes.reg, fractReg(varRes)];

  if(isConst) {
    const valueFP32 = f32ToFP32(varRight.value);
    const valInt = ((valueFP32 >>> 16) & 0xFFFF);
    const valFract = (valueFP32 & 0xFFFF);

    const opLoadInt = [
      ...(valInt === 0 ? [] : opsScalar.loadImmediate(REG.AT, valInt)),
      asm("mtc2", [valInt === 0 ? REG.ZERO : REG.AT, regDst[0] + swizzleRes]),
    ];
    const opLoadFract = [
      ...(valFract === 0 ? [] : opsScalar.loadImmediate(REG.AT, valFract)),
      asm("mtc2", [valFract === 0 ? REG.ZERO : REG.AT, regDst[1] + swizzleRes]),
    ];

    return varRes.type === "vec32" ? [...opLoadInt, ...opLoadFract] : opLoadInt;
  }

  if(isScalar) {
    if(varRes.type === "vec16") {
      return [asm("mtc2", ["at", regDst[0] + swizzleRes])];
    }
    return [
      asm("mtc2", [varRight.reg, regDst[1] + swizzleRes]),
      asm("srl", [REG.AT, varRight.reg, 16]),
      asm("mtc2", [REG.AT, regDst[0] + swizzleRes])
    ];
  }

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];

  if(varRes.type === "vec32") {
    return [
      asm("vmov", [regDst[0] + swizzleRes,       varRight.reg + swizzleRight]),
      asm("vmov", [regDst[1] + swizzleRes, fractReg(varRight) + swizzleRight])
    ];
  }
  return [asm("vmov", [regDst[0] + swizzleRes, varRight.reg + swizzleRight])];
}

function opLoad(varRes, varLoc, varOffset, swizzle)
{
  const res = [];

  if(swizzle && swizzle !== "xyzwxyzw") {
    state.throwError("Builtin load() only support '.xyzwxyzw' swizzle!", varRes);
  }
  if(!varLoc.reg) {
    if(varLoc.name) {
      res.push(...opsScalar.loadImmediate(REG.AT, "%lo(" +varLoc.name + ")"));
      varLoc = {type: "u32", reg: REG.AT};
    } else {
      state.throwError("Load base-address must be a variable!");
    }
  }
  if(isVecReg(varLoc.reg))state.throwError("Load base-address must be a scalar register!", varRes);
  if(varOffset.type !== "num")state.throwError("Load offset must be a numerical-constant!");

  let destOffset = varRes.swizzle ? SWIZZLE_SCALAR_IDX[varRes.swizzle] : 0;
  if(destOffset === undefined) {
    state.throwError("Unsupported destination swizzle (must be scalar .x to .W)!", varRes);
  }
  destOffset *= 2; // convert to bytes (?)

  const loadInstr = swizzle ? "ldv" : "lqv";

  res.push(           asm(loadInstr, [varRes.reg, toHex(destOffset), varOffset.value, varLoc.reg]));
  if(swizzle)res.push(asm(loadInstr, [varRes.reg, toHex(destOffset+8), varOffset.value, varLoc.reg]));

  if(varRes.type === "vec32") {
    res.push(           asm(loadInstr, [nextVecReg(varRes.reg), toHex(destOffset), varOffset.value + " + 0x10", varLoc.reg]));
    if(swizzle)res.push(asm(loadInstr, [nextVecReg(varRes.reg), toHex(destOffset+8), varOffset.value + " + 0x10", varLoc.reg]));
  }
  return res;
}

function opStore(varRes, varOffsets)
{
  const varLoc = state.getRequiredVarOrMem(varOffsets[0].value, "base");
  if(varOffsets.length > 1) {
    state.throwError("Vector stores can only use a single variable as the offset");
  }
  const opsLoad = [];
  if(!varLoc.reg) {
    varLoc.reg = REG.AT;
    opsLoad.push(...opsScalar.loadImmediate(varLoc.reg, "%lo(" +varLoc.name + ")"));
  }

  const is32 = (varRes.type === "vec32");
  return [...opsLoad,
          asm("sqv", [           varRes.reg,  "0x0", "0x00", varLoc.reg]),
   is32 ? asm("sqv", [nextVecReg(varRes.reg), "0x0", "0x10", varLoc.reg]) : null
  ];
}

function opAdd(varRes, varLeft, varRight)
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

function opSub(varRes, varLeft, varRight)
{
  if(!varRight.reg) {
    state.throwError("Subtraction cannot be done with a constant!");
  }
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Subtraction only allows swizzle on the right side!");
  }

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  return (varRes.type === "vec32")
    ? [
      asm("vsubc", [nextReg(varRes.reg), fractReg(varLeft), fractReg(varRight) + swizzleRight]),
      asm("vsub",  [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
    ] : [
      asm("vsubc", [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
    ];
}

function opMul(varRes, varLeft, varRight, clearAccum)
{
  if(!varRight.reg) {
    varRight = POW2_SWIZZLE_VAR[varRight.value];
    if(!varRight) {
      state.throwError("Multiplication by a constant can only be done with powers of two!");
    }
  }
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Multiplication only allows swizzle on the right side!");
  }

  const res = [];
  const right32Bit = varRight.type === "vec32";
  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  const fractOp = clearAccum ? "vmudl" : "vmadl";
  let intOp = clearAccum ? "vmudn": "vmadn";

  if(right32Bit) {
    res.push(
      asm(fractOp, [REG.VTEMP0, fractReg(varLeft), fractReg(varRight) + swizzleRight]),
      asm("vmadm", [REG.VTEMP0,       varLeft.reg, fractReg(varRight) + swizzleRight]),
    );
    intOp = "vmadn"; // don't clear inbetween
  }

  res.push(
    asm(intOp,   [nextVecReg(varRes.reg), fractReg(varLeft),       varRight.reg + swizzleRight]),
    asm("vmadh", [           varRes.reg,        varLeft.reg,       varRight.reg + swizzleRight]),
  );
  return res;
}

function opInvertHalf(varRes, varLeft) {

  if(!varLeft.swizzle && !varRes.swizzle) {
    const res = [];
    for(const s of Object.keys(SWIZZLE_SCALAR_IDX)) {
      res.push(...opInvertHalf({...varRes, swizzle: s}, {...varLeft, swizzle: s}));
    }
    return res;
  }

  if(!varLeft.swizzle || !varRes.swizzle) {
    state.throwError("Builtin invert() needs swizzling either on both sides or none (e.g.: 'res.y = invert(res).x')!", varRes);
  }
  if(!isScalarSwizzle(varRes.swizzle) || !isScalarSwizzle(varLeft.swizzle)) {
    return state.throwError("Swizzle on both sides must be single-lane! (.x to .W)");
  }

  const swizzleRes = SWIZZLE_MAP[varRes.swizzle || ""];
  const swizzleArg = SWIZZLE_MAP[varLeft.swizzle || ""];

  return [
    asm("vrcph", [      intReg(varRes) + swizzleRes,   intReg(varLeft) + swizzleArg]),
    asm("vrcpl", [    fractReg(varRes) + swizzleRes, fractReg(varLeft) + swizzleArg]),
    asm("vrcph", [      intReg(varRes) + swizzleRes,        REG.VZERO + swizzleArg]),
  ];
}

function opDiv(varRes, varLeft, varRight) {
  if(!varRight.swizzle || !isScalarSwizzle(varRight.swizzle)) {
    state.throwError("Vector division needs swizzling on the right side (must be scalar .x to .W)!", varRes);
  }
  const tmpVar = {type: varRight.type, reg: REG.VTEMP1};
  const tmpVarSw = {...tmpVar, swizzle: varRight.swizzle};

  return [
    ...opInvertHalf(tmpVarSw, varRight),
    ...opMul(tmpVar, tmpVar, POW2_SWIZZLE_VAR["2"], true),
    ...opMul(varRes, varLeft, tmpVarSw, true)
  ];
}

export default {opMove, opLoad, opStore, opAdd, opSub, opMul, opInvertHalf, opDiv};
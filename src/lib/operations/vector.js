/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {fractReg, getVec32Regs, intReg, isVecReg, nextReg, nextVecReg, REG} from "../syntax/registers";
import state from "../state";
import {
  isScalarSwizzle,
  POW2_SWIZZLE_VAR,
  SWIZZLE_MAP,
  SWIZZLE_MAP_KEYS_STR,
  SWIZZLE_SCALAR_IDX
} from "../syntax/swizzle";
import {f32ToFP32} from "../dataTypes/dataTypes.js";
import {asm} from "../intsructions/asmWriter.js";
import opsScalar from "./scalar";

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opMove(varRes, varRight)
{
  const isVec32 = varRes.type === "vec32";

  if(!varRight.reg && !varRes.swizzle) {
    if(varRight.value === 0) {
      return [    asm("vxor", [  intReg(varRes),   intReg(varRes),   intReg(varRes)]),
        isVec32 ? asm("vxor", [fractReg(varRes), fractReg(varRes), fractReg(varRes)]) : null
      ];
    }
    return state.throwError("Can only assign the scalar '0' to a non-swizzled vector!\n"
      + "If you want to load a single element, use 'foo.x = "+varRight.value+";' instead"
    );
  }

  const isConst = !varRight.reg;
  const isScalar = isConst || !varRight.type.startsWith("vec");

  if(varRes.swizzle) {
    if(!isScalarSwizzle(varRes.swizzle) || (!isScalar && !isScalarSwizzle(varRight.swizzle))) {
      return state.throwError("Vector swizzle must be single-lane! (.x to .W)");
    }
  }

  const swizzleRes = SWIZZLE_MAP[varRes.swizzle || ""];

  let regDst = getVec32Regs(varRes);
  let regsR = getVec32Regs(varRight);
  if(varRes.castType && varRight.castType) {
    regDst = [varRes.reg, REG.VZERO];
    regsR = [varRight.reg, REG.VZERO];
  }

  // Assigning an int or float constant to a vector
  if(isConst) {
    // if the constant is a power of two, use the special vector reg to avoid a load...
    const pow2 = POW2_SWIZZLE_VAR[varRight.value];
    if(pow2) {
      return [asm("vmov", [regDst[0] + swizzleRes, pow2.reg + SWIZZLE_MAP[pow2.swizzle]]),
              asm("vxor", [regDst[1], regDst[1], regDst[1]]) // clear fractional part
      ];
    }
    // ...otherwise load the constant into a scalar register and move
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

    return isVec32 ? [...opLoadInt, ...opLoadFract] : opLoadInt;
  }

  // Assigning a scalar value from a register to a vector
  if(isScalar) {
    if(varRes.type === "vec16") {
      return [asm("mtc2", [varRight.reg, regDst[0] + swizzleRes])];
    }
    return [
      asm("mtc2", [varRight.reg, regDst[1] + swizzleRes]),
      asm("srl", [REG.AT, varRight.reg, 16]),
      asm("mtc2", [REG.AT, regDst[0] + swizzleRes])
    ];
  }

  // moving an entire vector from A to B
  if(!varRight.swizzle) {
    return [asm("vor", [regDst[0], REG.VZERO, regsR[0]]),
            asm("vor", [regDst[1], REG.VZERO, regsR[1]])];
  }

  // moving a single element/lane form A to B
  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  return [asm("vmov", [regDst[0] + swizzleRes, regsR[0] + swizzleRight]),
          asm("vmov", [regDst[1] + swizzleRes, regsR[1] + swizzleRight])];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLoc
 * @param {ASTFuncArg} varOffset
 * @param {string} swizzle
 * @param {boolean} isPackedByte
 * @param {boolean} isSigned
 * @returns {ASM[]}
 */
function opLoad(varRes, varLoc, varOffset, swizzle, isPackedByte = false, isSigned = true)
{
  const res = [];

  if(swizzle && SWIZZLE_SCALAR_IDX[swizzle[0]] === undefined) {
    state.throwError("Unsupported load swizzle, this is a bug in RSPL, please let me know :/", varRes);
  }
  if(!varLoc.reg) {
    if(varLoc.name) {
      res.push(...opsScalar.loadImmediate(REG.AT, "%lo(" +varLoc.name + ")"));
      varLoc = {type: "num", reg: REG.AT};
    } else {
      state.throwError("Load base-address must be a variable!");
    }
  }
  if(isVecReg(varLoc.reg))state.throwError("Load base-address must be a scalar register!", varRes);
  if(varOffset.type !== "num")state.throwError("Load offset must be a numerical-constant!");

  let destOffset = varRes.swizzle ? SWIZZLE_SCALAR_IDX[varRes.swizzle[0]] : 0;
  if(destOffset === undefined) {
    state.throwError("Unsupported destination swizzle, this is a bug in RSPL, please let me know :/", varRes);
  }
  destOffset *= 2; // convert to bytes

  const is32 = (varRes.type === "vec32");
  const dupeLoad = swizzle === "xyzwxyzw"; // this loads the same data twice, into the lower/upper half
  if(dupeLoad)swizzle = "xyzw"; // @TODO: use a less hacky way

  let accessLen = swizzle ? (swizzle.length*2) : 16;
  let loadInstr = {1: "lbv", 2: "lsv", 4: "llv", 8: "ldv", 16: "lqv"}[accessLen];
  let srcOffset = swizzle ? (SWIZZLE_SCALAR_IDX[swizzle[0]] * 2) : 0;

  if(isPackedByte) {
    if(is32)state.throwError("Packed byte loads are not supported for 32-bit vectors!");
    loadInstr = isSigned ? "lpv" : "luv";
    srcOffset /= 2;
    destOffset /= 2;
  }

  srcOffset += varOffset.value;

  res.push(            asm(loadInstr, [varRes.reg, destOffset,   srcOffset, varLoc.reg]));
  if(dupeLoad)res.push(asm(loadInstr, [varRes.reg, destOffset+8, srcOffset, varLoc.reg]));

  if(is32) {
    res.push(            asm(loadInstr, [nextVecReg(varRes.reg), destOffset,   srcOffset + accessLen, varLoc.reg]));
    if(dupeLoad)res.push(asm(loadInstr, [nextVecReg(varRes.reg), destOffset+8, srcOffset + accessLen, varLoc.reg]));
  }
  return res;
}

function opLoadBytes(varRes, varLoc, varOffset, swizzle, isSigned) {
  return opLoad(varRes, varLoc, varOffset, swizzle, true, isSigned);
}

function opStore(varRes, varOffsets, isPackedByte = false, isSigned = true)
{
  if(varOffsets.length < 1)state.throwError("Vector stores need at least one offset / more than 1 argument!");
  const varLoc = state.getRequiredVarOrMem(varOffsets[0].value, "base");

  const opsLoad = [];
  if(!varLoc.reg) {
    varLoc.reg = REG.AT;
    opsLoad.push(...opsScalar.loadImmediate(varLoc.reg, "%lo(" +varLoc.name + ")"));
  }

  let baseOffset = 0;
  const offsets = varOffsets.slice(1);
  for(let offset of offsets) {
    if(offset.type !== "num")state.throwError("Vector stores can only use numerical constants as offsets");

    baseOffset += offset.value;
  }

  const is32 = (varRes.type === "vec32");
  const swizzle = varRes.swizzle;
  const accessLen = swizzle ? (swizzle.length*2) : 16;
  let storeInstr = {1: "sbv", 2: "ssv", 4: "slv", 8: "sdv", 16: "sqv"}[accessLen];
  let srcOffset = swizzle ? (SWIZZLE_SCALAR_IDX[swizzle[0]] * 2) : 0;

  if(isPackedByte) {
    if(is32)state.throwError("Packed byte stores are not supported for 32-bit vectors!");
    if(swizzle && swizzle.length !== 1)state.throwError("Packed byte stores only support single-lane swizzles! (.x to .W)");

    storeInstr = isSigned ? "spv" : "suv";
    srcOffset /= 2;
  }

  return [...opsLoad,
          asm(storeInstr, [           varRes.reg,  srcOffset, baseOffset              , varLoc.reg]),
   is32 ? asm(storeInstr, [nextVecReg(varRes.reg), srcOffset, baseOffset + accessLen, varLoc.reg]) : null
  ];
}

function opStoreBytes(varRes, varLoc, isSigned) {
  return opStore(varRes, varLoc, true, isSigned);
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opAdd(varRes, varLeft, varRight)
{
  if(!varRight.reg) {
    state.throwError("Addition cannot be done with a constant!");
  }
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Addition only allows swizzle on the right side!");
  }

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  if(swizzleRight === undefined) {
    state.throwError("Unsupported swizzle (supported: "+SWIZZLE_MAP_KEYS_STR+")!", varRes);
  }

  return (varRes.type === "vec32")
    ? [
      asm("vaddc", [nextReg(varRes.reg), fractReg(varLeft), fractReg(varRight) + swizzleRight]),
      asm("vadd",  [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
    ] : [
      asm("vaddc", [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
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
  if(!varRight.reg) {
    state.throwError("Subtraction cannot be done with a constant!");
  }
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Subtraction only allows swizzle on the right side!");
  }

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  if(swizzleRight === undefined) {
    state.throwError("Unsupported swizzle (supported: "+SWIZZLE_MAP_KEYS_STR+")!", varRes);
  }

  return (varRes.type === "vec32")
    ? [
      asm("vsubc", [nextReg(varRes.reg), fractReg(varLeft), fractReg(varRight) + swizzleRight]),
      asm("vsub",  [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
    ] : [
      asm("vsubc", [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
    ];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @param {string} op
 * @returns {ASM[]}
 */
function genericLogicOp(varRes, varLeft, varRight, op) {
  const funcName = op.toUpperCase().substring(1);
  if(!varRight.reg)state.throwError(funcName + " cannot be done with a constant!");
  if(varRes.swizzle || varLeft.swizzle)state.throwError(funcName + " only allows swizzle on the right side!");

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  if(swizzleRight === undefined) {
    state.throwError("Unsupported swizzle (supported: "+SWIZZLE_MAP_KEYS_STR+")!", varRes);
  }

  const is32 = (varRes.type === "vec32");
  return [asm(op, [        varRes.reg,       varLeft.reg,       varRight.reg  + swizzleRight]),
   is32 ? asm(op, [nextReg(varRes.reg), fractReg(varLeft), fractReg(varRight) + swizzleRight]) : null,
  ];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opAnd(varRes, varLeft, varRight) {
  return genericLogicOp(varRes, varLeft, varRight, "vand");
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opOr(varRes, varLeft, varRight) {
  return genericLogicOp(varRes, varLeft, varRight, "vor");
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opXOR(varRes, varLeft, varRight) {
  return genericLogicOp(varRes, varLeft, varRight, "vxor");
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opShiftLeft(varRes, varLeft, varRight) {
  return state.throwError("Shift-Left is not supported for vectors! (@TODO: implement this)");
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opShiftRight(varRes, varLeft, varRight) {
  return state.throwError("Shift-Right is not supported for vectors! (@TODO: implement this)");
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opBitFlip(varRes, varRight) {
  if(varRight.swizzle)state.throwError("NOT operator is only supported for variables!");
  return genericLogicOp(varRes, varRight, {type: 'u32', reg: REG.VZERO}, "vnor");
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @param {boolean} clearAccum
 * @returns {ASM[]}
 */
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

  if(swizzleRight === undefined) {
    state.throwError("Unsupported swizzle (supported: "+SWIZZLE_MAP_KEYS_STR+")!", varRes);
  }

  if(right32Bit) {
    res.push(
      asm(fractOp, [REG.VTEMP0, fractReg(varLeft), fractReg(varRight) + swizzleRight]),
      asm("vmadm", [REG.VTEMP0,       varLeft.reg, fractReg(varRight) + swizzleRight]),
    );
    intOp = "vmadn"; // don't clear inbetween
  } else if(varRes.type === "vec16") {
    return [
      asm(intOp, [varRes.reg, varLeft.reg, varRight.reg + swizzleRight]),
    ];
  }

  res.push(
    asm(intOp,   [nextVecReg(varRes.reg), fractReg(varLeft),       varRight.reg + swizzleRight]),
    asm("vmadh", [           varRes.reg,        varLeft.reg,       varRight.reg + swizzleRight]),
  );
  return res;
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @returns {ASM[]}
 */
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

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
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

export default {
  opMove, opLoad, opStore,
  opAdd, opSub, opMul, opInvertHalf, opDiv, opAnd, opOr, opXOR, opBitFlip,
  opShiftLeft, opShiftRight,
  opLoadBytes, opStoreBytes
};
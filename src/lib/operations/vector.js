/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {
  fractReg,
  getVec32Regs,
  intReg,
  isVecReg,
  LABELS,
  nextReg,
  nextVecReg,
  REG as REGS,
  REG, REG_COP2
} from "../syntax/registers";
import state from "../state";
import {
  isScalarSwizzle,
  POW2_SWIZZLE_VAR,
  SWIZZLE_MAP,
  SWIZZLE_MAP_KEYS_STR,
  SWIZZLE_SCALAR_IDX
} from "../syntax/swizzle";
import {f32ToFP32, isTwoRegType, isVecType} from "../dataTypes/dataTypes.js";
import {asm, asmNOP} from "../intsructions/asmWriter.js";
import opsScalar from "./scalar";
import builtins from "../builtins/functions.js";

/**
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 */
function assertVectorVars(varLeft, varRight) {
  if(!varLeft.type.startsWith("vec") || (varRight.reg && !varRight.type.startsWith("vec"))) {
    state.throwError("Vector-Operation requires all variables to be vectors!");
  }
}

/**
 * Function to handle all possible forms of direct assignment.
 * Scalar to vector, vector to vector, immediate-values, single lane-access, casting, ...
 * @param {ASTFuncArg} varRes target
 * @param {ASTFuncArg} varRight source
 * @returns {ASM[]}
 */
function opMove(varRes, varRight)
{
  const isVec32 = varRes.type === "vec32";

  if(!varRight.reg && !varRes.swizzle) {
    const swizzleVar = POW2_SWIZZLE_VAR[varRight.value];
    if(!swizzleVar) {
      return state.throwError("Can only assign a constant to a vector if it is a power of two or zero!\n"
           + "If you want to load a single element, use 'foo.x = "+varRight.value+";' instead",
            varRight
      );
    }

    return [    asm("vxor", [  intReg(varRes), REGS.VZERO, swizzleVar.reg + SWIZZLE_MAP[swizzleVar.swizzle]]),
      isVec32 ? asm("vxor", [fractReg(varRes), REGS.VZERO, REGS.VZERO]) : null
    ];
  }

  const isConst = !varRight.reg;
  const isScalar = isConst || !varRight.type.startsWith("vec");

  let isHalfMove = false;
  if(varRes.swizzle) {
    const isNonScalar = !isScalarSwizzle(varRes.swizzle) || (!isScalar && !isScalarSwizzle(varRight.swizzle));
    isHalfMove = varRes.swizzle.length === 4 && (varRight.swizzle || "").length === 4;

    if(isNonScalar && !isHalfMove) {
      return state.throwError("Vector swizzle must be single-lane, or half-lane! (.x to .W, or .xyzw / .XYZW)");
    }
  }

  let swizzleRes = SWIZZLE_MAP[varRes.swizzle || ""];

  let regDst = getVec32Regs(varRes);
  let regsR = getVec32Regs(varRight);
  if(varRes.castType && varRight.castType) {
    regDst = [varRes.reg, REG.VZERO];
    regsR = [varRight.reg, REG.VZERO];
  }

  // Assigning an integer or float constant to a vector
  if(isConst) {
    if(isHalfMove)state.throwError("Half-moves are not supported for constants!");

    // if the constant is a power of two, use the special vector reg to avoid a load...
    const regPow2 = POW2_SWIZZLE_VAR[varRight.value];
    const regZero = POW2_SWIZZLE_VAR[0]; // assigning an int to a vec32 needs to clear th fraction to zero
    if(regPow2) {
      return [asm("vmov", [regDst[0] + swizzleRes, regPow2.reg + SWIZZLE_MAP[regPow2.swizzle]]),
              asm("vmov", [regDst[1] + swizzleRes, regZero.reg + SWIZZLE_MAP[regZero.swizzle]]),
      ];
    }
    // ...otherwise load the constant into a scalar register and move

    const isFractCast = ["ufract", "sfract"].includes(varRes.castType);
    const valueFP32 = f32ToFP32(varRight.value * (varRes.castType === "sfract" ? 0.5 : 1.0));
    let valInt = ((valueFP32 >>> 16) & 0xFFFF);
    let valFract = (valueFP32 & 0xFFFF);

    if(varRes.castType === "sfract" && varRight.value >= 0) {
      valFract = Math.min(0x7FFF, valFract);
    }

    if(isFractCast)valInt = valFract;

    const opLoadInt = [
      ...(valInt === 0 ? [] : opsScalar.loadImmediate(REG.AT, valInt)),
      asm("mtc2", [valInt === 0 ? REG.ZERO : REG.AT, regDst[0] + swizzleRes]),
    ];
    const opLoadFract = [
      ...(valFract === 0 ? [] : opsScalar.loadImmediate(REG.AT, valFract)),
      asm("mtc2", [valFract === 0 ? REG.ZERO : REG.AT, regDst[1] + swizzleRes]),
    ];

    if(isVec32) {
      return [...opLoadInt, ...opLoadFract];
    }
    return isFractCast ? opLoadFract : opLoadInt;
  }

  // Assigning a scalar value from a register to a vector
  if(isScalar) {
    if(isHalfMove)state.throwError("Half-moves are not supported for scalar values!");

    // if we want to set a scalar to an entire vector, we need to expand it
    // this assigns it first to a single lane, then expands into the entire vector
    const needsExpand = !varRes.swizzle;
    if(needsExpand)swizzleRes = SWIZZLE_MAP.x;

    if(varRes.type === "vec16") {
      return [
        asm("mtc2", [varRight.reg, regDst[0] + swizzleRes]),
        needsExpand ? asm("vor", [regDst[0], REGS.VZERO, regDst[0] + swizzleRes]) : null,
      ];
    }
    return [
      asm("mtc2", [varRight.reg, regDst[1] + swizzleRes]),
      asm("srl", [REG.AT, varRight.reg, 16]),
      asm("mtc2", [REG.AT, regDst[0] + swizzleRes]),
      needsExpand ? asm("vor", [regDst[0], REGS.VZERO, regDst[0] + swizzleRes]) : null,
      needsExpand ? asm("vor", [regDst[1], REGS.VZERO, regDst[1] + swizzleRes]) : null,
    ];
  }

  // moving an entire vector from A to B
  if(!varRight.swizzle) {
    return [asm("vor", [regDst[0], REG.VZERO, regsR[0]]),
            asm("vor", [regDst[1], REG.VZERO, regsR[1]])];
  }

  // broadcasting 4 lanes to the other 4 lanes (aka half-vector move)
  if(isHalfMove) {
    state.addAnnotation("Barrier", "__SCRATCH_MEM__");
    return [
      ...opStore(varRight, [{type: "num", value: LABELS.RSPQ_SCRATCH_MEM}], false, false),
      ...opLoad(varRes, {reg: REG.AT}, {type: "num", value: 0}, "xyzw", false, false),
    ];
  }

  // broadcasting a swizzle into a complete vector
  if(!varRes.swizzle) {
    const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
    return [asm("vor", [regDst[0], REG.VZERO, regsR[0] + swizzleRight]),
            asm("vor", [regDst[1], REG.VZERO, regsR[1] + swizzleRight])];
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
function opLoad(varRes, varLoc, varOffset, swizzle, isPackedByte = false, isSigned = true, isUnaligned = false)
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
  } else if(varOffset.name) {
    // now we want to do a load based on a variable with and label-offset
    res.push(...opsScalar.opAdd({reg: REG.AT}, {reg: varLoc.reg}, {value: varOffset.name}));
    varOffset = {type: "num", value: 0};
    varLoc = {type: "num", reg: REG.AT};
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

  let alignLoadOp = (isUnaligned && loadInstr === 'lqv') ? "lrv" : undefined;

  srcOffset += varOffset.value;

  if(loadInstr === "lqv" && srcOffset % 16 !== 0) {
    state.throwError("Invalid full vector-load offset, must be a multiple of 16, " + srcOffset + " given", varRes);
  }

  res.push(            asm(loadInstr, [varRes.reg, destOffset,   srcOffset, varLoc.reg]));
  if(dupeLoad)res.push(asm(loadInstr, [varRes.reg, destOffset+8, srcOffset, varLoc.reg]));

  if(alignLoadOp) {
    res.push(            asm(alignLoadOp, [varRes.reg, destOffset,   srcOffset+0x10, varLoc.reg]));
    if(dupeLoad)res.push(asm(alignLoadOp, [varRes.reg, destOffset+8, srcOffset+0x10, varLoc.reg]));
  }

  if(is32) {
    res.push(            asm(loadInstr, [nextVecReg(varRes.reg), destOffset,   srcOffset + accessLen, varLoc.reg]));
    if(dupeLoad)res.push(asm(loadInstr, [nextVecReg(varRes.reg), destOffset+8, srcOffset + accessLen, varLoc.reg]));

    if(alignLoadOp) {
      res.push(            asm(alignLoadOp, [nextVecReg(varRes.reg), destOffset,   srcOffset + accessLen+0x10, varLoc.reg]));
      if(dupeLoad)res.push(asm(alignLoadOp, [nextVecReg(varRes.reg), destOffset+8, srcOffset + accessLen+0x10, varLoc.reg]));
    }
  }
  return res;
}

function opLoadBytes(varRes, varLoc, varOffset, swizzle, isSigned) {
  return opLoad(varRes, varLoc, varOffset, swizzle, true, isSigned);
}

function opStore(varRes, varOffsets, isPackedByte = false, isSigned = true, isUnaligned = false)
{
  if(varOffsets.length < 1)state.throwError("Vector stores need at least one offset / more than 1 argument!");
  const varLoc = state.getRequiredVarOrMem(varOffsets[0].value, "base");
  if(isVecType(varLoc.type) && !varLoc.arraySize) {
    state.throwError("store base-addresses must be in a scalar register!", varLoc);
  }

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

  const alignOp = (isUnaligned && storeInstr === 'sqv') ? "srv" : undefined;

  return [...opsLoad,
             asm(storeInstr, [           varRes.reg,  srcOffset, baseOffset            , varLoc.reg]),
   alignOp ? asm(alignOp,    [           varRes.reg,  srcOffset, baseOffset + 0x10     , varLoc.reg]) : null,
   is32    ? asm(storeInstr, [nextVecReg(varRes.reg), srcOffset, baseOffset + accessLen, varLoc.reg]) : null,
   is32 && alignOp
           ? asm(alignOp,    [nextVecReg(varRes.reg), srcOffset, baseOffset + accessLen + 0x10, varLoc.reg]) : null,
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
    varRight = POW2_SWIZZLE_VAR[varRight.value];
    if(!varRight) {
      state.throwError(
        "Addition by a constant can only be done with powers of two!\n" +
        "Assign any other values to a vector-variable first.\n" +
        "E.g. 'vec16 add; add.x = 42;'"
      );
    }
  }
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Addition only allows swizzle on the right side!");
  }
  assertVectorVars(varLeft, varRight);

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  if(swizzleRight === undefined) {
    state.throwError("Unsupported swizzle (supported: "+SWIZZLE_MAP_KEYS_STR+")!", varRes);
  }
  const regsDst = getVec32Regs(varRes);
  const regsL = getVec32Regs(varLeft);
  const regsR = getVec32Regs(varRight);

  //let fractOp = ["sfract", "ufract"].includes(varRes.castType) ? "vadd" : "vaddc";
  let fractOp = "vaddc";
  if(varRes.castType) {
    fractOp = ["sfract"].includes(varRes.castType) ? "vadd" : "vaddc";
  }

  let intOp = varRes.castType === "sint" ? "vadd" : "vaddc";
  if(varRes.type === "vec32") {
    fractOp = "vaddc"; intOp = "vadd";
  }

  return [
    asm(fractOp, [regsDst[1], regsL[1], regsR[1] + swizzleRight]),
    asm(intOp,   [regsDst[0], regsL[0], regsR[0] + swizzleRight]),
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
    varRight = POW2_SWIZZLE_VAR[varRight.value];
    if(!varRight) {
      state.throwError("Subtraction by a constant can only be done with powers of two!");
    }
  }
  if(varRes.swizzle || varLeft.swizzle) {
    state.throwError("Subtraction only allows swizzle on the right side!");
  }
  assertVectorVars(varLeft, varRight);

  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  if(swizzleRight === undefined) {
    state.throwError("Unsupported swizzle (supported: "+SWIZZLE_MAP_KEYS_STR+")!", varRes);
  }

  return (varRes.type === "vec32")
    ? [
      asm("vsubc", [nextReg(varRes.reg), fractReg(varLeft), fractReg(varRight) + swizzleRight]),
      asm("vsub",  [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
    ] : [
      (varRes.castType && varRes.castType.startsWith("s"))
       ? asm("vsub", [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight])
       : asm("vsubc", [        varRes.reg,        varLeft.reg,      varRight.reg  + swizzleRight]),
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
  assertVectorVars(varLeft, varRight);

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
function opNOR(varRes, varLeft, varRight) {
  return genericLogicOp(varRes, varLeft, varRight, "vnor");
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
  if(typeof(varRight.value) === "string")state.throwError("Shift-Left cannot use labels!");
  if(varRight.reg)state.throwError("Vector-Shift amount must be a constant,\nfor variable shifts use multiplications instead");

  if(varRight.value < 0 || varRight.value > 31) {
    state.throwError("Shift-Left value must be in range 0<x<32!");
  }

  const shiftVal = Math.floor(Math.pow(2, varRight.value));
  const shiftReg = POW2_SWIZZLE_VAR[shiftVal];
  if(!shiftReg)state.throwError(`Invalid shift value (${varRight.value} -> V:${shiftVal})`, varRight);

  const regR = shiftReg.reg + SWIZZLE_MAP[shiftReg.swizzle];

  if(varRes.type === "vec32") {
    const regsRes = getVec32Regs(varRes);
    const regsL = getVec32Regs(varLeft);

    const firstReg = (regsRes[0] === regsL[0]) ? REG.VTEMP0 : regsRes[0];

    return [
      asm("vmudl", [firstReg,   regsL[1], regR]),
      asm("vmadn", [regsRes[0], regsL[0], regR]),
      asm("vmudn", [regsRes[1], regsL[1], regR])
    ];
  }

  if(varRes.type === "vec16" && varLeft.type === 'vec32') {
    const regsL = getVec32Regs(varLeft);
    return [
      asm("vmudl", [varRes.reg, regsL[1], regR]),
      asm("vmadn", [varRes.reg, regsL[0], regR]),
    ];
  }

  if(varRes.type !== varLeft.type) {
    state.throwError("Cannot left-shift vec16 into vec32!");
  }

  return [
    asm("vmudn", [varRes.reg, varLeft.reg, regR])
  ];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @param {boolean} logical
 * @returns {ASM[]}
 */
function opShiftRight(varRes, varLeft, varRight, logical)
{
  if(typeof(varRight.value) === "string")state.throwError("Shift-Right cannot use labels!");
  if(varRight.reg)state.throwError("Vector-Shift amount must be a constant,\nfor variable shifts use multiplications instead");

  if(varRight.value < 0 || varRight.value > 31) {
    state.throwError("Shift-Right value must be in range 0<x<32!");
  }

  const shiftVal = Math.floor(1 / Math.pow(2, varRight.value) * 0x10000);
  const shiftReg = POW2_SWIZZLE_VAR[shiftVal];
  if(!shiftReg)state.throwError(`Invalid shift value (${varRight.value} -> V:${shiftVal})`, varRight);

  if(varRes.type === "vec32") {
    const regsRes = getVec32Regs(varRes);
    const regsL = getVec32Regs(varLeft);
    let instMid;

    // s16 shift into s16.16 result
    if(regsL[1] === REG.VZERO) {
      instMid = logical ? "vmudn" : "vmudm";
      return [
        asm(instMid, [regsRes[0], regsL[0], shiftReg.reg + SWIZZLE_MAP[shiftReg.swizzle]]),
        asm("vmadn", [regsRes[1], REGS.VZERO, REGS.VZERO]),
      ];
    }

    // s16.s16 shift into s16.16 result
    instMid = logical ? "vmadn" : "vmadm";
    return [
      asm("vmudl", [regsRes[1], regsL[1], shiftReg.reg + SWIZZLE_MAP[shiftReg.swizzle]]),
      asm(instMid, [regsRes[0], regsL[0], shiftReg.reg + SWIZZLE_MAP[shiftReg.swizzle]]),
      asm("vmadn", [regsRes[1], REGS.VZERO, REGS.VZERO]),
    ];
  }

  return [logical
    ? asm("vmudl", [varRes.reg, varLeft.reg, shiftReg.reg + SWIZZLE_MAP[shiftReg.swizzle]])
    : asm("vmudm", [varRes.reg, varLeft.reg, shiftReg.reg + SWIZZLE_MAP[shiftReg.swizzle]])
  ];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opBitFlip(varRes, varRight) {
  if(varRight.swizzle)state.throwError("NOT operator is only supported for variables!");
  return genericLogicOp(varRes, varRight, {type: 'vec16', reg: REG.VZERO}, "vnor");
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
  assertVectorVars(varLeft, varRight);

  const res = [];
  const right32Bit = varRight.type === "vec32";
  const swizzleRight = SWIZZLE_MAP[varRight.swizzle || ""];
  const fractOp = clearAccum ? "vmudl" : "vmadl";
  let intOp = clearAccum ? "vmudn": "vmadn";

  if(swizzleRight === undefined) {
    state.throwError("Unsupported swizzle (supported: "+SWIZZLE_MAP_KEYS_STR+")!", varRes);
  }

  // special-case: multiplying a s1.15 with a 0.16 (fraction of a s16.16)
  // @TODO: refactor
  if(varRes.type === "vec16") {
    if(varLeft.type === "vec16" && ["sfract", "ufract"].includes(varLeft.castType)) {
      if(varRight.originalType === "vec32" && ["sfract", "ufract"].includes(varRight.castType)) {
        let opMid = clearAccum ? "vmudm": "vmadm";
        return [asm(opMid, [varRes.reg, varLeft.reg, varRight.reg + swizzleRight])];
      }
    }
  }

   if(varRes.originalType === "vec32" && varRes.castType === "sfract"
    && varLeft.type === "vec32" && varRight.type === "vec32"
   ) {
      return [
        asm("vmudl", [REG.VTEMP0, fractReg(varLeft), fractReg(varRight) + swizzleRight]),
        asm("vmadm", [REG.VTEMP0,   intReg(varLeft), fractReg(varRight) + swizzleRight]),
        asm("vmadn", [varRes.reg, fractReg(varLeft),   intReg(varRight) + swizzleRight]),
      ];
  }

  let varRightHigh = varRight.reg + swizzleRight;

  // @TODO: refactor
  const resRegs = getVec32Regs(varRes);

  // 16bit * 16bit multiply
  if(varRes.type === "vec32" && varLeft.type === "vec16" && varRight.type === "vec16")
  {
    // integer multiply into a 32bit vector
    if(!["sfract", "ufract"].includes(varLeft.castType) &&
       !["sfract", "ufract"].includes(varRight.castType))
    {
      const intOp = clearAccum ? "vmudh": "vmadh";
      return [
        asm(intOp,  [resRegs[1], varLeft.reg, varRight.reg + swizzleRight]),
        asm("vsar", [resRegs[0], REG_COP2.ACC_HI]),
        asm("vsar", [resRegs[1], REG_COP2.ACC_MD]),
      ];
    }
  }

  // 16bit * 32bit multiply
  if(right32Bit && varRes.type === "vec32" && varLeft.type === "vec16" && !["sfract", "ufract"].includes(varLeft.castType)) {
    return [
      asm("vmudm", [resRegs[1], varLeft.reg, fractReg(varRight) + swizzleRight]),
      asm("vmadh", [resRegs[0], varLeft.reg, intReg(varRight) + swizzleRight]),
      asm("vmadn", [resRegs[1], REGS.VZERO, REGS.VZERO]),
    ];
  }

  const rightSideIsFraction = ["sfract", "ufract"].includes(varRight.castType);

  // Full 32-bit multiplication
  if(right32Bit) {
    res.push(
      asm(fractOp, [REG.VTEMP0, fractReg(varLeft), fractReg(varRight) + swizzleRight]),
      asm("vmadm", [REG.VTEMP0,       varLeft.reg, fractReg(varRight) + swizzleRight]),
    );
    intOp = "vmadn"; // don't clear inbetween
  } // Partial multiplication: s16.16 * 0.16 (fractional part of original s16.16)
  else if(rightSideIsFraction && (varRight.originalType === "vec32" || varRes.type === "vec32"))
  {
    res.push(
      asm(fractOp, [nextVecReg(varRes.reg), fractReg(varLeft), varRight.reg + swizzleRight]),
      asm("vmadm", [           varRes.reg,        varLeft.reg, varRight.reg + swizzleRight]),
      asm("vmadn", [nextVecReg(varRes.reg), REGS.VZERO,        REGS.VZERO]),
    );
    return res;
  } // 16-Bit multiplication
  else if(varRes.type === "vec16")
  {
    const caseRef = varLeft.castType || varRight.castType || varRes.castType;
    if(caseRef === "ufract" || caseRef === "sfract")
    {
      intOp = clearAccum ? "vmul": "vmac";
      intOp += (caseRef === "ufract") ? "u" : "f";
      return [asm(intOp, [varRes.reg, varLeft.reg, varRight.reg + swizzleRight])];
    }

    if(varLeft.castType === "sint" || varRight.castType === "sint") {
      intOp = clearAccum ? 'vmudh' : 'vmadh';
    }

    return [asm(intOp, [varRes.reg, varLeft.reg, varRight.reg + swizzleRight])];
  }

  const regResFract = resRegs[1] === REGS.VZERO ? REGS.VTEMP0 : resRegs[1];
  res.push(
    asm(intOp,   [regResFract, fractReg(varLeft), varRightHigh]),
    asm("vmadh", [ varRes.reg,        varLeft.reg, varRightHigh]),
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

  if(varRes.type === "vec32" && varLeft.type === "vec16") {
    if(varLeft.castType === "ufract" || varLeft.castType === "sfract") {
      state.throwError("invert() is not supported for 16-bit fractional types! (@TODO impl.)");
    }
    return [
      asm("vrcp",  [fractReg(varRes) + swizzleRes, varLeft.reg + swizzleArg]),
      asm("vrcph", [  intReg(varRes) + swizzleRes, varLeft.reg + swizzleArg]),
    ];
  }

  if(varRes.type === 'vec16' && (varRes.castType === 'sfract' || varRes.castType === 'ufract')) {
    return [
      asm("vrcph", [ REG.VTEMP0 + swizzleRes,   intReg(varLeft) + swizzleArg]),
      asm("vrcpl", [ varRes.reg + swizzleRes, fractReg(varLeft) + swizzleArg]),
    ];
  }

  return [
    asm("vrcph", [      intReg(varRes) + swizzleRes,   intReg(varLeft) + swizzleArg]),
    asm("vrcpl", [    fractReg(varRes) + swizzleRes, fractReg(varLeft) + swizzleArg]),
    asm("vrcph", [      intReg(varRes) + swizzleRes,        REG.VZERO + swizzleArg]),
  ];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @returns {ASM[]}
 */
function opInvertSqrtHalf(varRes, varLeft) {

  if(!varLeft.swizzle && !varRes.swizzle) {
    const res = [];
    for(const s of Object.keys(SWIZZLE_SCALAR_IDX)) {
      res.push(...opInvertSqrtHalf({...varRes, swizzle: s}, {...varLeft, swizzle: s}));
    }
    return res;
  }

  if(!varLeft.swizzle || !varRes.swizzle) {
    state.throwError("Builtin invert_sqrt() needs swizzling either on both sides or none (e.g.: 'res.y = invert_sqrt(res).x')!", varRes);
  }
  if(!isScalarSwizzle(varRes.swizzle) || !isScalarSwizzle(varLeft.swizzle)) {
    return state.throwError("Swizzle on both sides must be single-lane! (.x to .W)");
  }

  const swizzleRes = SWIZZLE_MAP[varRes.swizzle || ""];
  const swizzleArg = SWIZZLE_MAP[varLeft.swizzle || ""];

  return [
    asm("vrsqh", [      intReg(varRes) + swizzleRes,   intReg(varLeft) + swizzleArg]),
    asm("vrsql", [    fractReg(varRes) + swizzleRes, fractReg(varLeft) + swizzleArg]),
    asm("vrsqh", [      intReg(varRes) + swizzleRes,        REG.VZERO + SWIZZLE_MAP.x]),
  ];
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function opDiv(varRes, varLeft, varRight) {
  state.throwError("Vector division is not supported! Use invert_half() or shift-right instead.");
}

/**
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @param {CompareOp} op
 * @param {?ASTTernary} ternary
 * @returns {ASM[]}
 */
function opCompare(varRes, varLeft, varRight, op, ternary) {
  if(!ternary && isTwoRegType(varRes.type))state.throwError("Vector comparison can only use vec16!", varRes);
  if(varLeft.type !== "vec16")state.throwError("Vector comparison can only use vec16!", varLeft);
  if(varRight.type !== "vec16")state.throwError("Vector comparison can only use vec16!", varRight);
  if(varRes.swizzle)state.throwError("Vector comparison result variable cannot use swizzle!", varRes);
  if(varLeft.swizzle)state.throwError("Vector comparison left-side cannot use swizzle!", varLeft);

  const ops = {
    "<":  "vlt",
    "==": "veq",
    "!=": "vne",
    ">=":  "vge",
  };
  const opInstr = ops[op];
  if(!opInstr)state.throwError(`Unsupported comparison operator: ${op}, allowed: ${Object.keys(ops).join(", ")}`);

  let swizzleRight = "";
  if(varRight.swizzle) {
    swizzleRight = SWIZZLE_MAP[varRight.swizzle];
    if(!swizzleRight)state.throwError("Unsupported swizzle (supported: "+SWIZZLE_MAP_KEYS_STR+")!", varRes);
  }

  let regCompareDst = ternary ? REG.VTEMP0 : varRes.reg;
  let res = [asm(opInstr, [regCompareDst, varLeft.reg, varRight.reg + swizzleRight])];

  if(ternary) {
    res.push(...builtins.select(varRes, [
      {type: "var", value: ternary.left},
      {type: typeof(ternary.right) === "number" ? "num" : "var", value: ternary.right, swizzle: ternary.swizzleRight},
    ], undefined));
  }
  return res;
}

export default {
  opMove, opLoad, opStore,
  opAdd, opSub, opMul, opInvertHalf, opInvertSqrtHalf, opDiv, opAnd, opOr, opNOR, opXOR, opBitFlip,
  opShiftLeft, opShiftRight,
  opLoadBytes, opStoreBytes,
  opCompare,
};
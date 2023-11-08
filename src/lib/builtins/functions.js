/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {nextReg, REG, REGS_VECTOR} from "../syntax/registers";
import state from "../state";
import opsScalar from "../operations/scalar";
import opsVector from "../operations/vector";
import {asm, asmNOP} from "../intsructions/asmWriter.js";
import {isTwoRegType, isVecType, TYPE_SIZE} from "../types/types.js";
import {SWIZZLE_MAP} from "../syntax/swizzle.js";
import {DMA_FLAGS} from "./libdragon.js";

function assertArgsNoSwizzle(args, offset = 0) {
  args = args.slice(offset);
  for(const arg of args) {
    if(arg.swizzle)state.throwError(offset > 0
      ? `Only the first ${offset} argument(s) can use swizzling!`
      : "Arguments with swizzle not allowed in this function!",
    arg);
  }
}

function load(varRes, args, swizzle)
{
  assertArgsNoSwizzle(args);
  if(!varRes)state.throwError("Builtin load() needs a left-side", varRes);
  if(args.length === 1) {
    args = [args[0], {type: "num", value: 0}];
  }

  const argVar = state.getRequiredVarOrMem(args[0].value, "arg0");
  const argOffset = (args[1].type === "num")
    ? args[1]
    : state.getRequiredMem(args[1].value, "arg1");

  const isVector = REGS_VECTOR.includes(varRes.reg);
  if(!isVector) {
    return opsScalar.opLoad(varRes, argVar, argOffset);
  }

  return opsVector.opLoad(varRes, argVar, argOffset, swizzle);
}

function store(varRes, args, swizzle)
{
  assertArgsNoSwizzle(args, 1);
  if(varRes)state.throwError("Builtin store() cannot have a left side!\nUsage: 'store(varToSave, address, optionalOffset);'", varRes);
  const varSrc = state.getRequiredVar(args[0].value, "arg0");
  varSrc.swizzle = args[0].swizzle;

  const isVectorSrc = REGS_VECTOR.includes(varSrc.reg);

  if(swizzle)state.throwError("Builtin store() cannot use swizzle!");

  if(isVectorSrc) {
    return opsVector.opStore(varSrc, args.slice(1));
  }

  if(varSrc.swizzle)state.throwError("Scalar variables cannot use swizzling!", varSrc);
  return opsScalar.opStore(varSrc, args.slice(1));
}

function load_vec_u8(varRes, args, swizzle, isSigned = false) {
  assertArgsNoSwizzle(args);
  if(args.length < 1)state.throwError("Builtin loadVecU8() requires at least one argument!", args[0]);
  if(!varRes)state.throwError("Builtin loadVecU8() needs a left-side", varRes);
  if(!isVecType(varRes.type))state.throwError("Builtin loadVecU8() must store the result into a vector!", varRes);

  if(args.length === 1) {
    args = [args[0], {type: "num", value: 0}];
  }

  const argVar = state.getRequiredVarOrMem(args[0].value, "arg0");
  const argOffset = (args[1].type === "num")
    ? args[1] : state.getRequiredMem(args[1].value, "arg1");

  return opsVector.opLoadBytes(varRes, argVar, argOffset, swizzle, isSigned);
}

function load_vec_s8(varRes, args, swizzle) {
  return load_vec_u8(varRes, args, swizzle, true);
}

function store_vec_u8(varRes, args, swizzle, isSigned = false) {
  assertArgsNoSwizzle(args, 1);
  if(varRes)state.throwError("Builtin store_vec_x8() cannot have a left side!\nUsage: 'store_vec_u8(varToSave, address, optionalOffset);'", varRes);
  const varSrc = state.getRequiredVar(args[0].value, "arg0");
  varSrc.swizzle = args[0].swizzle;

  const isVectorSrc = REGS_VECTOR.includes(varSrc.reg);

  if(swizzle)state.throwError("Builtin store_vec_x8() cannot use swizzle!");

  if(isVectorSrc) {
    return opsVector.opStoreBytes(varSrc, args.slice(1), isSigned);
  }
}

function store_vec_s8(varRes, args, swizzle) {
  return store_vec_u8(varRes, args, swizzle, true);
}

function inlineAsm(varRes, args, swizzle) {
  assertArgsNoSwizzle(args);
  if(swizzle)state.throwError("Builtin asm() cannot use swizzle!", varRes);
  if(varRes)state.throwError("Builtin asm() cannot have a left side!", varRes);
  if(args.length !== 1 || args[0].type !== "string") {
    state.throwError("Builtin asm() requires exactly one string argument!", args[0]);
  }
  return [asm(args[0].value, ["# inline-ASM"])];
}

// Taken from libdragon
function DMA_SIZE(width, height) {
  return (((width)-1) | (((height)-1)<<12)) >>> 0;
}

function genericDMA(varRes, args, swizzle, builtinName, dmaName) {
  assertArgsNoSwizzle(args);
  if(swizzle)state.throwError("Builtin "+builtinName+"() cannot use swizzle!", varRes);
  if(varRes)state.throwError("Builtin "+builtinName+"() cannot have a left side!", varRes);
  if(args.length !== 2 && args.length !== 3)state.throwError("Builtin "+builtinName+"() requires 2 or 3 arguments!", args[0]);

  const targetMem = state.getRequiredVarOrMem(args[0].value, "dest");
  const varRDRAM = state.getRequiredVar(args[1].value, "RDRAM");

  if(targetMem.reg && args.length !== 3) {
    state.throwError("Builtin "+builtinName+"() requires size-argument when using a variable as destination!", args[0]);
  }

  let sizeLoadOps = [];

  // explicit size is set, can be either a constant or a variable...
  if(args.length === 3)
  {
    const sizeArg = args[2];
    if(sizeArg.type === "num") {
      sizeLoadOps = [asm("ori", [REG.T0, REG.ZERO, DMA_SIZE(sizeArg.value, 1)])];
    } else {
      const sizeVar = state.getRequiredVar(sizeArg.value, "size");
      if(sizeVar.reg !== REG.T0)state.throwError("Builtin "+builtinName+"() requires size-argument to be in $t0!", sizeArg);
      sizeLoadOps = [asm("addiu", [REG.T0, REG.T0, -1])]; // part of the DMA_SIZE calculation, assuming height=1
    }

    if(targetMem.reg) {
      if(targetMem.reg !== REG.S4)state.throwError("Builtin "+builtinName+"() requires dest. var to be in $s4!", args);
    } else {
      sizeLoadOps.push(asm("ori", [REG.S4, REG.ZERO, "%lo("+targetMem.name+")"]));
    }

  } else { // ...as a fallback, use the declared state size (incl. array size)
    const targetSize = TYPE_SIZE[targetMem.type] * (targetMem.arraySize || 1);
    sizeLoadOps = [
      asm("ori", [REG.T0, REG.ZERO, DMA_SIZE(targetSize, 1)]),
      asm("ori", [REG.S4, REG.ZERO, "%lo("+targetMem.name+")"]),
    ];
  }

  return [
    varRDRAM.reg === REG.S0 ? null : asm("or", [REG.S0, REG.ZERO, varRDRAM.reg]),
    ...sizeLoadOps,
    ...opsScalar.loadImmediate(REG.T2, DMA_FLAGS[dmaName]),
    asm("jal", ["DMAExec"]),
    asmNOP(),
  ];
}

function dma_in(varRes, args, swizzle) {
  return genericDMA(varRes, args, swizzle, "dma_in", "DMA_IN");
}

function dma_in_async(varRes, args, swizzle) {
  return genericDMA(varRes, args, swizzle, "dma_in_async", "DMA_IN_ASYNC");
}

function dma_out(varRes, args, swizzle) {
  return genericDMA(varRes, args, swizzle, "dma_out", "DMA_OUT")
}

function dma_out_async(varRes, args, swizzle) {
  return genericDMA(varRes, args, swizzle, "dma_out_async", "DMA_OUT_ASYNC")
}

function dma_await(varRes, args, swizzle) {
  if(varRes)state.throwError("Builtin dma_await() cannot have a left side!", varRes);
  if(args.length > 0)state.throwError("Builtin dma_await() requires no arguments!", args);
  if(swizzle)state.throwError("Builtin dma_await() cannot use swizzle!", varRes);

  return [asm("jal", ["DMAWaitIdle"]), asmNOP()];
}

function invertHalf(varRes, args, swizzle) {
  assertArgsNoSwizzle(args);
  if(args.length !== 1)state.throwError("Builtin invertHalf() requires exactly one argument!", args[0]);
  const varArg = state.getRequiredVar(args[0].value, "arg0");
  if(!isVecType(varArg.type))state.throwError("Builtin invert() requires a vector argument!", args[0]);
  return opsVector.opInvertHalf(varRes, {...varArg, swizzle});
}

function invert(varRes, args, swizzle) {
  assertArgsNoSwizzle(args);
  if(swizzle)state.throwError("Builtin invert() cannot use swizzle, use invertHalf() instead and multiply manually", varRes);
  const res = invertHalf(varRes, args, swizzle);
  res.push(...opsVector.opMul(varRes, varRes, {type: "num", value: 2}, true));
  return res;
}

function int(varRes, args, swizzle) {
  assertArgsNoSwizzle(args);
  const varArg = state.getRequiredVar(args[0].value, "arg0");

  if(args.length !== 1      )state.throwError("Builtin int() requires exactly one argument!", args[0]);
  if(!varRes                )state.throwError("Builtin int() needs a left-side", varRes);
  if(isVecType(varRes.type) )state.throwError("Builtin int() must store the result into an integer!", varRes);
  if(!swizzle               )state.throwError("Builtin int() requires swizzling! (.x to .W)", varArg);
  if(swizzle.length !== 1   )state.throwError("Builtin int() swizzle must use a single element (.x to .W)", varArg);
  if(!isVecType(varArg.type))state.throwError("The argument of builtin int() must be a vector!", varArg);

  return [asm("mfc2", [varRes.reg, varArg.reg + SWIZZLE_MAP[swizzle]])];
}

function fract() {
  //assertArgsNoSwizzle(args);
  state.throwError("@TODO: Builtin fract() not implemented!");
}

function swap(varRes, args, swizzle) {
  assertArgsNoSwizzle(args);
  if(args.length !== 2)state.throwError("Builtin swap() requires exactly two argument!", args);
  if(swizzle)state.throwError("Builtin swap() cannot use swizzle!", varRes);
  if(varRes)state.throwError("Builtin swap() cannot have a left side!", varRes);

  const varA = state.getRequiredVar(args[0].value, "arg0");
  const varB = state.getRequiredVar(args[1].value, "arg1");

  if(varA.type !== varB.type)state.throwError("Builtin swap() requires both arguments to be of the same type!", args);
  if(varA.reg === varB.reg) {
    state.logWarning("Both arguments for swap() are using the same registers, ignore", args);
    return [];
  }

  const res = [];
  const xorOp = isVecType(varA.type) ? "vxor" : "xor";

  res.push(
    asm(xorOp, [varA.reg, varA.reg, varB.reg]),
    asm(xorOp, [varB.reg, varA.reg, varB.reg]),
    asm(xorOp, [varA.reg, varA.reg, varB.reg])
  );

  if(isTwoRegType(varA.type)) {
    const regA = nextReg(varA.reg);
    const regB = nextReg(varB.reg);

    res.push(
      asm(xorOp, [regA, regA, regB]),
      asm(xorOp, [regB, regA, regB]),
      asm(xorOp, [regA, regA, regB])
    );
  }

  return res;
}

export default {
  load, store, load_vec_u8, load_vec_s8, store_vec_u8, store_vec_s8,
  asm: inlineAsm,
  dma_in, dma_out, dma_in_async, dma_out_async, dma_await,
  invertHalf, invert, int, fract, swap
};
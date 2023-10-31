/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {fractReg, intReg, nextVecReg, REG, REGS_VECTOR} from "../syntax/registers";
import state from "../state";
import opsScalar from "../operations/scalar";
import opsVector from "../operations/vector";
import {asm, asmNOP} from "../intsructions/asmWriter.js";
import {TYPE_SIZE} from "../types/types.js";
import {isScalarSwizzle, SWIZZLE_MAP, SWIZZLE_SCALAR_IDX} from "../syntax/swizzle.js";

function load(varRes, args, swizzle)
{
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
  if(varRes)state.throwError("Builtin store() cannot have a left side!\nUsage: 'store(varToSave, address, optionalOffset);'", varRes);

  const varSrc = state.getRequiredVar(args[0].value, "arg0");
  const isVectorSrc = REGS_VECTOR.includes(varSrc.reg);

  if(swizzle)state.throwError("Builtin store() cannot use swizzle!");

  if(isVectorSrc) {
    return opsVector.opStore(varSrc, args.slice(1));
  }
  return opsScalar.opStore(varSrc, args.slice(1));
}

function inlineAsm(varRes, args, swizzle) {
  if(swizzle)state.throwError("Builtin asm() cannot use swizzle!", varRes);
  if(varRes)state.throwError("Builtin asm() cannot have a left side!", varRes);
  if(args.length !== 1 || args[0].type !== "string") {
    state.throwError("Builtin asm() requires exactly one string argument!", args[0]);
  }
  return [asm(args[0].value, ["# inline-ASM"])];
}

function genericDMA(varRes, args, swizzle, builtinName, dmaName) {
  if(swizzle)state.throwError("Builtin "+builtinName+"() cannot use swizzle!", varRes);
  if(varRes)state.throwError("Builtin "+builtinName+"() cannot have a left side!", varRes);
  if(args.length !== 2)state.throwError("Builtin "+builtinName+"() requires exactly two arguments!", args[0]);

  const targetMem = state.getRequiredMem(args[0].value, "dest");
  const targetSize = TYPE_SIZE[targetMem.type] * (targetMem.arraySize || 1);

  const varRDRAM = state.getRequiredVar(args[1].value, "RDRAM");

  return [
    varRDRAM.reg === REG.S0 ? null : asm("or", [REG.S0, REG.ZERO, varRDRAM.reg]),
    asm("ori", [REG.T0, REG.ZERO, "DMA_SIZE("+targetSize+", 1)"]),
    asm("ori", [REG.S4, REG.ZERO, "%lo("+targetMem.name+")"]),
    asm("jal", [dmaName]),
    asmNOP(),
  ];
}

function dma_in(varRes, args, swizzle) {
  return genericDMA(varRes, args, swizzle, "dma_in", "DMAIn");
}

function dma_out(varRes, args, swizzle) {
  return genericDMA(varRes, args, swizzle, "dma_out", "DMAOut")
}

function invertHalf(varRes, args, swizzle) {
  if(args.length !== 1)state.throwError("Builtin invertHalf() requires exactly one argument!", args[0]);
  const varArg = state.getRequiredVar(args[0].value, "arg0");
  return opsVector.opInvertHalf(varRes, {...varArg, swizzle});
}

export default {load, store, asm: inlineAsm, dma_in, dma_out, invertHalf};
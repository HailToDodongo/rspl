/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {nextVecReg, REG, REGS_VECTOR} from "../syntax/registers";
import state from "../state";
import opsScalar from "../operations/scalar";
import opsVector from "../operations/vector";
import {asm, asmNOP} from "../intsructions/asmWriter.js";
import {TYPE_SIZE} from "../types/types.js";

function load(varRes, args, swizzle)
{
  if(!varRes)state.throwError("Builtin load() needs a left-side", varRes);
  if(args.length === 1)args = [args[0], {type: "num", value: 0}];

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
  if(!varRes)state.throwError("Builtin store() needs a left-side", varRes);
  if(args.length === 1)args = [args[0], {type: "num", value: 0}];

  const varSrc = state.getRequiredVar(args[0].value, "arg0");
  const isVectorSrc = REGS_VECTOR.includes(varSrc.reg);
  const isVectorDst = REGS_VECTOR.includes(varRes.reg);

  if(isVectorDst)state.throwError("Builtin store(), left side must be a scalar register!", varRes);
  if(swizzle)state.throwError("Builtin store() cannot use swizzle!", varRes);

  if(isVectorSrc) {
    const is32 = (varSrc.type === "vec32");

    return [asm("sqv", [           varSrc.reg,  "0x0", "0x00", varRes.reg]),
     is32 ? asm("sqv", [nextVecReg(varSrc.reg), "0x0", "0x10", varRes.reg]) : null
    ];
  }

  return opsScalar.opStore(varSrc, varRes, args.slice(1));
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

export default {load, store, asm: inlineAsm, dma_in, dma_out};
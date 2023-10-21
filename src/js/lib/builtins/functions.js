/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {nextVecReg, REGS_VECTOR} from "../syntax/registers";
import state from "../state";
import {toHex} from "../types/types";
import {SWIZZLE_SCALAR_IDX} from "../syntax/swizzle";

function load(varRes, args, swizzle)
{
  const isVector = REGS_VECTOR.includes(varRes.reg);
  if(!isVector)state.throwError("Builtin load() only works for vectors!", varRes);

  if(swizzle && swizzle !== "xyzwxyzw") {
    state.throwError("Builtin load() only support '.xyzwxyzw' swizzle!", varRes);
  }
  const expand = swizzle === "xyzwxyzw";

  const argVar = state.getRequiredVar(args[0].value, "arg0");
  const argOffset = args[1] ? parseInt(args[1].value) : 0;

  let destOffset = varRes.swizzle ? SWIZZLE_SCALAR_IDX[varRes.swizzle] : 0;
  if(destOffset === undefined) {
    state.throwError("Unsupported destination swizzle (must be scalar .x to .W)!", varRes);
  }
  destOffset *= 2; // to bytes (?)
  const loadInstr = expand ? "ldv" : "lqv";

  const res = [];

  res.push([loadInstr, varRes.reg, toHex(destOffset,2), toHex(argOffset), argVar.reg]);
  if(expand) {
    res.push([loadInstr, varRes.reg, toHex(destOffset+8,2), toHex(argOffset), argVar.reg]);
  }

  if(varRes.type === "vec32") {
    res.push([loadInstr, nextVecReg(varRes.reg), toHex(destOffset,2), toHex(argOffset+0x10), argVar.reg]);
    if(expand) {
      res.push([loadInstr, nextVecReg(varRes.reg), toHex(destOffset+8,2), toHex(argOffset+0x10), argVar.reg]);
    }
  }
  return res;
}

function store(varRes, args, swizzle)
{
  if(args.length !== 1)state.throwError("Builtin store() requires exactly one argument (vector variable)!", varRes);

  const varSrc = state.getRequiredVar(args[0].value, "arg0");
  const isVectorSrc = REGS_VECTOR.includes(varSrc.reg);
  const isVectorDst = REGS_VECTOR.includes(varRes.reg);

  if(!isVectorSrc)state.throwError("Builtin store() only works for vectors!", varRes);
  if(isVectorDst)state.throwError("Builtin store(), left side must be a scalar register!", varRes);
  if(swizzle)state.throwError("Builtin store() cannot use swizzle!", varRes);

  const is32 = (varSrc.type === "vec32");

  return [["sqv",            varSrc.reg,  "0x0", "0x00", varRes.reg],
   is32 ? ["sqv", nextVecReg(varSrc.reg), "0x0", "0x10", varRes.reg] : []
  ];
}

export default {load, store};
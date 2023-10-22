/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {nextVecReg, REGS_VECTOR} from "../syntax/registers";
import state from "../state";
import {toHex} from "../types/types";
import {SWIZZLE_SCALAR_IDX} from "../syntax/swizzle";
import opsScalar from "../operations/scalar";
import opsVector from "../operations/vector";

function load(varRes, args, swizzle)
{
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
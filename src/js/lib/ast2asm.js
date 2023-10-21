/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import opsScalar from "./operations/scalar";
import opsVector from "./operations/vector";
import state from "./state";

const VECTOR_TYPES = ["vec16", "vec32"];

function calcToAsm(calc, varRes)
{
  switch(calc.type)
  {
    case "calcVar": {
      const varRight = state.getRequiredVar(calc.right, "right", calc);
      varRight.swizzle = calc.swizzleRight;
      return calcAssignToAsm(calc, varRes, varRight);
    }

    case "calcNum": {
      const varRight = {type: varRes.type, value: calc.right};
      return calcAssignToAsm(calc, varRes, varRight);
    }

    case "calcVarVar": {
      const varLeft = state.getRequiredVar(calc.left, "Left", calc);
      const varRight = state.getRequiredVar(calc.right, "right", calc);
      varLeft.swizzle = calc.swizzleLeft;
      varRight.swizzle = calc.swizzleRight;

      return calcLRToAsm(calc, varRes, varLeft, varRight);
    }

    case "calcVarNum": {
      const varLeft = state.getRequiredVar(calc.left, "Left", calc);
      varLeft.swizzle = calc.swizzleLeft;
      const varRight = {type: varLeft.type, value: calc.right};

      return calcLRToAsm(calc, varRes, varLeft, varRight);
    }

    default: state.throwError("Unknown calculation type: " + calc.type, calc);
  }
}

function calcAssignToAsm(calc, varRes, varRight) {
  const isVector = VECTOR_TYPES.includes(varRes.type);
  const opsHandler = isVector ? opsVector : opsScalar;

  if(!isVector && (calc.swizzleLeft || calc.swizzleRight)) {
    state.throwError("Swizzling not allowed for scalar operations!");
  }

  return opsHandler.opMove(varRes, varRight);
}

function calcLRToAsm(calc, varRes, varLeft, varRight)
{
  const op = calc.op;
  if(varLeft.type !== varRight.type || varLeft.type !== varRes.type) {
    state.throwError("Type mismatch!");
  }

  const isVector = VECTOR_TYPES.includes(varRes.type);
  const opsHandler = isVector ? opsVector : opsScalar;

  if(!isVector && (calc.swizzleLeft || calc.swizzleRight)) {
    state.throwError("Swizzling not allowed for scalar operations!");
  }

  switch (op) {
    case  "+":  return opsHandler.opAdd(varRes, varLeft, varRight, true);
    case "++":  return opsHandler.opAdd(varRes, varLeft, varRight, false);
    case  "*":  return opsHandler.opMul(varRes, varLeft, varRight, true);
    case "+*":  return opsHandler.opMul(varRes, varLeft, varRight, false);

    case "&":  return opsHandler.opAnd(varRes, varLeft, varRight);

    case "<<":  return opsHandler.opShiftLeft(varRes, varLeft, varRight);
    case ">>":  return opsHandler.opShiftRight(varRes, varLeft, varRight);

    default: state.throwError("Unknown operator: " + op);
  }
}

function functionToAsm(func, args)
{
  const res = [];


  const declareVar = (name, type, reg) => {
    // @TODO: check for conflicts
    state.varMap[name] = {reg, type};
    state.regVarMap[reg] = name;
  };

  let argIdx = 0;
  for(const arg of args) {
    declareVar(arg.name, arg.type, "$a"+argIdx);
    ++argIdx;
  }

  for(const st of func.statements) 
  {
    state.line = st.line || 0;

    switch(st.type) 
    {
      case "comment":
        res.push(["##" + (st.comment || "")]);
      break;

      case "asm":
        res.push([st.asm]);
      break;

      case "varDecl":
        declareVar(st.varName, st.varType, st.reg);
        break;

      case "varAssignCalc": {
        const calc = st.calc;
        const varRes = structuredClone(state.varMap[st.varName]);
        varRes.swizzle = st.swizzle;
        if(!varRes)state.throwError("Destination Variable "+st.varName+" not known!", st);

        res.push(...calcToAsm(calc, varRes));
      } break;

      case "funcCall": {
        if(st.func === "asm") {
          const asm = st.args.value;
          res.push([asm.substring(1, asm.length-1)]); // remove quotes
        } else {
          state.throwError("Unknown function: " + st.func, st);
        }
      } break;

      default:
        res.push(["#### UNKNOWN: " + JSON.stringify(st)])
    }
  }
  return res;
}

function getArgSize(block)
{
  if(block.type !== "command")return 0;
  // each arg is always 4-bytes, the first one is implicitly set
  return Math.max(block.args.length * 4, 4);
}

export function ast2asm(ast)
{
  const res = [];

  for(const block of ast.functions)
  {
    state.func = block.name || "";
    state.line = block.line || 0;

    if(["function", "command"].includes(block.type)) {
      state.enterFunction(block.name);

      res.push({
        ...block,
        asm: functionToAsm(block.body, block.args),
        argSize: getArgSize(block),
        body: undefined
      });
    }
  }
  return res;
}

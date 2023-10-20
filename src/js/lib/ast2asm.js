/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import opsScalar from "./operations/scalar";
import opsVector from "./operations/vector";
import state from "./state";

const VECTOR_TYPES = ["vec16", "vec32"];

const toHex16 = v => "0x" + v.toString(16).padStart(4, '0').toUpperCase();
const toHex32 = v => "0x" + v.toString(16).padStart(8, '0').toUpperCase();

function calcToAsm(calc, varRes, varLeft, varRight)
{
  const op = calc.op;
  if(varLeft.type !== varRight.type || varLeft.type !== varRes.type) {
    state.throwError("Type mismatch!");
  }

  const opsHandler = VECTOR_TYPES.includes(varRes.type) ? opsVector : opsScalar;

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
  
  const varMap = {};
  const regVarMap = {};

  const declareVar = (name, type, reg) => {
    // @TODO: check for conflicts
    varMap[name] = {reg, type};
    regVarMap[reg] = name;
  };

  let argIdx = 0;
  for(const arg of args) {
    console.log("arg", arg);
    declareVar(arg.name, arg.type, "$a"+argIdx);
    ++argIdx;
  }

  for(const st of func.statements) 
  {
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

      case "varAssign": {
        if(st.value.type !== "value") {
          throw new Error("Invalid value! " + JSON.stringify(st));
        }

        const refVar = varMap[st.varName];
        const val = st.value.value;
        res.push(["li", refVar.reg, val > 0xFFFF ? toHex32(val.toString(16)) : `%lo(${toHex16(val)})`]);
      } break;

      case "varAssignCalc": {
        const calc = st.calc;
        const varRes   = structuredClone(varMap[st.varName]);
        const varLeft  = structuredClone(varMap[calc.left]);

        if(!varRes)state.throwError("Destination Variable "+st.varName+" not known!", st);
        if(!varLeft)state.throwError("Left Variable "+calc.left+" not known!", st);

        let varRight = undefined;
        if(calc.type === "calcVarVar") {
          varRight = structuredClone(varMap[calc.left]);
          if(!varRight)state.throwError("Right Variable "+calc.right+" not known!", st);
        } else if(calc.type === "calcVarNum") {
          varRight = {type: varLeft.type, value: calc.right};
        } else {
          state.throwError("Unknown calculation type: " + calc.type, st);
        }

        varLeft.swizzle = calc.swizzleLeft;
        varRight.swizzle = calc.swizzleRight;
        res.push(...calcToAsm(calc, varRes, varLeft, varRight));
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
    if(["function", "command"].includes(block.type)) {
      console.log(block);

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

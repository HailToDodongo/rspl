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

function calcToAsm(op, varRes, varLeft, varRight)
{
  if(varLeft.type !== varRight.type || varLeft.type !== varRes.type) {
    state.throwError("Type mismatch!");
  }

  const opsHandler = VECTOR_TYPES.includes(varRes.type) ? opsVector : opsScalar;

  switch (op) {
    case  "+":  return opsHandler.opAdd(varRes, varLeft, varRight, true);
    case "++":  return opsHandler.opAdd(varRes, varLeft, varRight, false);
    case  "*":  return opsHandler.opMul(varRes, varLeft, varRight, true);
    case "+*":  return opsHandler.opMul(varRes, varLeft, varRight, false);
    default: state.throwError("Unknown operator: " + op);
  }
}

function functionToAsm(func)
{
  const res = [];
  
  const varMap = {};
  const regVarMap = {};

  for(const st of func.statements) 
  {
    switch(st.type) 
    {
      case "comment":
        res.push(["##" + (st.comment || "")]);
      break;

      case "varDecl":
        // @TODO: check for conflicts
        varMap[st.varName] = {reg: st.reg, type: st.varType};
        regVarMap[st.reg] = st.varName;
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
        const varRight = structuredClone(varMap[calc.right]);

        if(!varRes)state.throwError("Destination Variable "+st.varName+" not known!", st);
        if(!varLeft)state.throwError("Left Variable "+calc.left+" not known!", st);
        if(!varRight)state.throwError("Right Variable "+calc.right+" not known!", st);

        varLeft.swizzle = calc.swizzleLeft;
        varRight.swizzle = calc.swizzleRight;
        res.push(...calcToAsm(calc.op, varRes, varLeft, varRight));
      } break;

      default:
        res.push(["#### UNKNOWN: " + JSON.stringify(st)])
    }
  }
  return res;
}

export function ast2asm(ast)
{
  const res = [];
  console.log(ast);

  for(const block of ast.functions)
  {
    if(block.type === "function") {
      console.log(block);

      res.push({
        type: "function",
        name: block.name,
        asm: functionToAsm(block.body)
      });
    }
  }

  return res;
}

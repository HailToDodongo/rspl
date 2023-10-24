/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {TYPE_REG_COUNT} from "./types/types";
import {nextReg} from "./syntax/registers";

function normalizeScopedBlock(block, astState)
{
  const statements = [];
  for(const st of block.statements)
  {
    switch (st.type)
    {
      case "scopedBlock":
        normalizeScopedBlock(st, astState);
        statements.push(st);
      break;

      case "if":
        normalizeScopedBlock(st.blockIf, astState);
        if(st.blockElse) {
          normalizeScopedBlock(st.blockElse, astState);
        }
        statements.push(st);
      break;

      // Split up declaration and assignment
      case "varDeclAssign":
        statements.push({...st, type: "varDecl"});
        if(st.calc) { // ... and ignore empty assignments
          statements.push({
            type: "varAssignCalc",
            varName: st.varName,
            calc: st.calc, assignType: "=",
          });
        }
      break;

      case "varDeclMulti":
        let regOffset = 0;
        for(const varName of st.varNames) {
          const reg = nextReg(st.reg, regOffset);
          statements.push({...st, varName, reg, type: "varDecl"});
          regOffset += TYPE_REG_COUNT[st.varType];
        }
      break;

      default: statements.push(st); break;

    }
  }

  for(const st of statements)
  {
    if(st.type === "varAssignCalc")
    {
      // convert constants from seemingly being variables to immediate-values
      // this changes the calc. type, instructions need to handle both numbers and strings
      if(["calcVar", "calcVarVar"].includes(st.calc.type)) {
        const stateVar = astState.find(s => s.varName === st.calc.right);
        if(stateVar) {
          st.calc.type = st.calc.type === "calcVar" ? "calcNum" : "calcVarNum";
          st.calc.right = `%lo(${st.calc.right})`;
        }
      }

      // Expand the short form of assignments/calculations (e.g: "a += b" -> "a = a + b")
      if(st.assignType !== "=") {
        const expOp = st.assignType.substring(0, st.assignType.length-1);
        st.calc.type = st.calc.type === "calcVar" ? "calcVarVar" : "calcVarNum";
        st.calc.left = st.varName;
        st.calc.swizzleLeft = undefined; // @TODO: handle this?
        st.calc.op = expOp;
        st.assignType = "=";
      }
    }
  }

  block.statements = statements;
}

export function astNormalizeFunctions(ast)
{
  const astFunctions = ast.functions;
  for(const block of astFunctions) {
    if(!["function", "command"].includes(block.type))continue;
    normalizeScopedBlock(block.body, ast.state);
  }

  return astFunctions;
}
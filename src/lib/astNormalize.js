/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {TYPE_REG_COUNT} from "./types/types";
import {nextReg} from "./syntax/registers";
import state from "./state.js";

function normalizeScopedBlock(block, astState, macros)
{
  const statements = [];
  for(const st of block.statements)
  {
    state.line = st.line || 0;

    switch (st.type)
    {
      case "scopedBlock":
        normalizeScopedBlock(st, astState, macros);
        statements.push(st);
      break;

      case "if":
        normalizeScopedBlock(st.blockIf, astState, macros);
        if(st.blockElse) {
          normalizeScopedBlock(st.blockElse, astState, macros);
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

      case "funcCall":
        if(macros[st.func]) {
          const macro = structuredClone(macros[st.func]);
          if(st.args.length !== macro.args.length) {
            state.throwError(`Macro '${st.func}' expects ${macro.args.length} arguments, got ${st.args.length}!`, st);
          }
          normalizeScopedBlock(macro.body, astState, macros);

          const varDecl = [];
          for(const [i, arg] of macro.args.entries())
          {
            varDecl.push({
              type: "varDeclAlias",
              varType: arg.type,
              varName: arg.name,
              aliasName: st.args[i].value,
            });
          }
          macro.body.statements = [...varDecl, ...macro.body.statements];
          statements.push(macro.body);

        } else {
          statements.push(st);
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
  const macros = {};

  for(const block of astFunctions) {
    if(!["function", "command", "macro"].includes(block.type) || !block.body)continue;

    if(block.type === "command" && block.resultType === null) {
      state.throwError("Commands must specify an index (e.g. 'command<4>')!", block)
    }
    if(block.type === "macro") {
      if(block.resultType != null) {
        state.throwError("Macros must not specify an result-type (use 'macro' without `< >`)!", block);
      }
      macros[block.name] = block;
    }
  }

  for(const block of astFunctions) {
    if(block.type !== "macro" && block.body) {
      state.func = block.name || "";
      normalizeScopedBlock(block.body, ast.state, macros);
    }
  }

  return astFunctions;
}
/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {TYPE_REG_COUNT} from "./dataTypes/dataTypes.js";
import {nextReg} from "./syntax/registers";
import state from "./state.js";
import {validateAnnotation} from "./syntax/annotations.js";
import builtins from "./builtins/functions.js";

/**
 * @param {ASTScopedBlock} block
 * @param {ASTState[]} astState
 * @param {ASTMacroMap} macros
 */
function normalizeScopedBlock(block, astState, macros)
{
  // convert labels we find into memory variables, this has the same effect as
  // globally setting them as "extern" in the RSPL state.
  for(const st of block.statements) {
    if(st.type === "labelDecl") {
      astState.push({
        arraySize: [1], extern: true, varType: 'u16',
        varName: st.name, align: 0, value: undefined
      });
      state.declareMemVar(st.name, 'u16', 1);
    }
  }

  /** @type {ASTStatement[]} */
  let statements = [];
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

      case "while":
      case "loop":
        normalizeScopedBlock(st.block, astState, macros);
        statements.push(st);
      break;

      // Split up declaration and assignment
      case "varDeclAssign":
        statements.push({...st, type: "varDecl", varName: st.varName.split(":")[0]});
        if(st.calc) { // ... and ignore empty assignments
          statements.push({
            type: "varAssignCalc",
            varName: st.varName,
            calc: st.calc,
            assignType: "=",
            line: st.line,
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

  // expand assigned function calls (if user defined)
  statements = statements.map(st =>
  {
    state.line = st.line || 0;
    if(st.type === "varAssignCalc" && st.calc.type === "calcFunc" && !builtins[st.calc.funcName])
    {
      if(st.swizzle || st.calc.swizzleRight) {
        state.throwError("Swizzle not allowed for user-defined functions!", st);
      }
      return {
        type: "funcCall",
        func: st.calc.funcName,
        args: [{type: "var", value: st.varName}, ...st.calc.args],
        line: st.line,
      };
    }
    return st;
  });

  statements = statements.map(st =>
  {
    state.line = st.line || 0;

    switch (st.type)
    {
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
              varName: st.args[i].value,
              aliasName: arg.name,
            });
          }
          macro.body.statements = [...varDecl, ...macro.body.statements];
          return macro.body;

        } else {
          return st;
        }
      break;
      default: return st;
    }
  });

  const newStm = [];
  for(const st of statements)
  {
    if(st.type === "varAssignCalc")
    {
      if(st.calc.type === "calcMulti")
      {
        if(st.assignType !== "=") {
          state.throwError("@TODO: multiple calculation in an increment not supported!", st);
        }

        console.log(st.calc.parts);
        let lastLeft = st.calc.left;
        let lastLeftSwizzle = st.swizzle;
        for(const part of st.calc.parts) {
          /** @type {ASTCalcLR} */
          const calcLR = {
            type: "calcLR",
            right: part.right,
            op: part.op,
            swizzleRight: part.swizzleRight,

            left: lastLeft,
            swizzleLeft: lastLeftSwizzle
          };

          const stateVar = astState.find(s => s.varName === part.right.value);
          if(stateVar) {
            //calcLR.type = "calcNum";
            calcLR.right = {type: 'num', value: `%lo(${part.right.value})`};
          }

          newStm.push({
            type: "varAssignCalc",
            varName: st.varName,
            calc: calcLR,
            assignType: "=",
            line: st.line,
          });
          // now use result as left side
          lastLeft = {type: 'VarName', value: st.varName};
          lastLeftSwizzle = st.swizzle;
          console.log(calcLR);
        }
        continue;
      }

      let isVarR = st.calc.type === "calcLR" || st.calc.type === "calcVar";
      // convert constants from seemingly being variables to immediate-values
      // this changes the calc. type, instructions need to handle both numbers and strings
      if(isVarR) {
        const stateVar = astState.find(s => s.varName === st.calc.right.value);
        if(stateVar) {
          st.calc.type = st.calc.type === "calcLR" ? "calcLR" : "calcNum";
          st.calc.right = {type: 'num', value: `%lo(${st.calc.right.value})`};
          isVarR = false;
        }
      } else {
        st.calc.right = {
          type: typeof(st.calc.right) === 'number' ? 'num' : 'VarName',
          value: st.calc.right
        };
      }

      // Expand the short form of assignments/calculations (e.g: "a += b" -> "a = a + b")
      if(st.assignType !== "=") {
        const expOp = st.assignType.substring(0, st.assignType.length-1);
        st.calc.type = 'calcLR';
        st.calc.left = {type: 'VarName', value: st.varName};

        if(!st.calc.right.type) {
          st.calc.right = isVarR
            ? {type: 'VarName', value: st.calc.right}
            : {type: 'num', value: st.calc.right};
        }
        st.calc.swizzleLeft = undefined; // @TODO: handle this?
        st.calc.op = expOp;
        st.assignType = "=";
      }

      // normalize cast-syntax, if the destination has a cast but the rest doesn't,
      // assume the L and R variable use the same by default
      /*if(['calcVarNum', 'calcVarVar'].includes(st.calc.type) && st.varName.includes(":"))
      {
        if(!st.calc.left.includes(":")) {
          st.calc.left += ":" + st.varName.split(":")[1];
        }
        if(st.calc.type === 'calcVarVar' && !st.calc.right.includes(":")) {
          st.calc.right += ":" + st.varName.split(":")[1];
        }
      }*/
    }
    newStm.push(st);
  }

  block.statements = newStm;
}

/**
 * @param {AST} ast
 * @returns {ASTFunc[]}
 */
export function astNormalizeFunctions(ast)
{
  const astFunctions = ast.functions;

  /** @type {ASTMacroMap} */
  const macros = {};

  for(const block of astFunctions) {
    if(!["function", "command", "macro"].includes(block.type) || !block.body)continue;

    for(const anno of block.annotations) {
      validateAnnotation(anno);
    }

    if(block.type === "command" && block.resultType === null) {
      state.throwError("Commands must specify an index (e.g. 'command<4>')!", block)
    }

    if(block.type === "macro") {
      if(block.resultType != null) {
        state.throwError("Macros must not specify an result-type (use 'macro' without `< >`)!", block);
      }
      if(builtins[block.name]) {
        state.throwError(`Macro '${block.name}' shadows a builtin function! Please use another name.`);
      }

      macros[block.name] = block;
    }
  }

  for(const block of astFunctions) {
    if(block.type !== "macro" && block.body) {
      state.func = block.name || "";
      normalizeScopedBlock(block.body, [...ast.state, ...ast.stateData, ...ast.stateBss], macros);
    }
  }

  return astFunctions;
}
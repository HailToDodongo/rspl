/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {TYPE_REG_COUNT} from "./dataTypes/dataTypes.js";
import {nextReg} from "./syntax/registers";
import state from "./state.js";
import {validateAnnotation} from "./syntax/annotations.js";
import builtins from "./builtins/functions.js";
import {astCalcNormalize} from "./astCalcNormalizer.js";

/**
 * 
 * @param {ASTStatement[]} statements 
 * @param {ASTStatement} currentStatement 
 * @returns {ASTStatement[]}
 */
function getPrecedingAnnotations(statements, currentStatement)
{
  const currentStatementIdx = statements.indexOf(currentStatement);
  if (currentStatement < 0)
    return [];

  /** @type {ASTStatement[]} */
  let annotations = [];
  // Search backwards from the current statement
  for (let index = currentStatementIdx-1; index >= 0; index--) {
    const st = statements[index];
    if (st.type !== 'annotation') break;
    annotations.splice(0, 0, st);
  }
  return annotations;
}

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
          // Duplicate annotations to the assigment
          statements.push(...getPrecedingAnnotations(block.statements, st));
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
        newStm.push({
          type: 'nestedCalc',
          line: st.line,
          varName: st.varName,
          swizzle: st.swizzle,
          parts: astCalcNormalize(st, astState)
        });
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
    }
    newStm.push(st);
  }

  block.statements = newStm;
}

/**
 * @param {AST} ast
 * @param {RSPLConfig} config
 * @returns {ASTFunc[]}
 */
export function astNormalizeFunctions(ast, config)
{
  const astFunctions = ast.functions;

  /** @type {ASTMacroMap} */
  const macros = {};
  let shaderCount = 0;

  for(const block of astFunctions) {
    if(["function", "command"].includes(block.type)) {
      ast.state.push({
        arraySize: [1], extern: true, varType: 'u16',
        varName: block.name, align: 0, value: undefined
      });
    }
  }

  for(const block of astFunctions) {
    if(!["function", "command", "macro", "shader"].includes(block.type) || !block.body)continue;

    for(const anno of block.annotations) {
      validateAnnotation(anno);
    }

    if(block.type === "command") {
      if(config.magma) {
        state.throwError("Commands must not be defined when compiling for magma (define a 'shader' instead)!", block);
      }
      if(block.resultType === null) {
        state.throwError("Commands must specify an index (e.g. 'command<4>')!", block);
      }
    }

    if(block.type === "macro") {
      if(block.resultType != null) {
        state.throwError("Macros must not specify a result-type (use 'macro' without `< >`)!", block);
      }
      if(builtins[block.name]) {
        state.throwError(`Macro '${block.name}' shadows a builtin function! Please use another name.`);
      }
      macros[block.name] = block;
    }

    if(block.type === "shader") {
      if(!config.magma) {
        state.throwError("Shaders are only allowed when compiling for magma (pass '--magma' on the command line)!", block);
      }
      if(shaderCount > 0) {
        state.throwError("A shader has already been defined!", block);
      }
      if(block.resultType != null) {
        state.throwError("Shaders must not specify a result-type (use 'shader' without `< >`)!", block);
      }
      if(block.args.length > 0) {
        state.throwError("Shaders must not specify arguments!", block);
      }
      shaderCount++;
    }
  }

  if(config.magma && shaderCount === 0) {
    state.throwError("Exactly one shader must be defined when compiling for magma (use 'shader')!");
  }

  for(const block of astFunctions) {
    if(block.type !== "macro" && block.body) {
      state.func = block.name || "";
      const uniformsState = ast.uniforms.flatMap(u => u.state);
      normalizeScopedBlock(block.body, [...ast.state, ...ast.stateData, ...ast.stateBss, ...uniformsState], macros);
    }
  }

  return astFunctions;
}

/**
 * @param {AST} ast
 * @param {RSPLConfig} config
 * @returns {ASTState[]}
 */
export function astNormalizeState(ast, config)
{
  const astState = ast.state;
  return astState;
}

/**
 * @param {AST} ast
 * @param {RSPLConfig} config
 * @returns {ASTUniform[]}
 */
export function astNormalizeUniforms(ast, config)
{
  const astUniforms = ast.uniforms;

  let curBindingNumber = 0;
  const usedBindingNumbers = new Set(astUniforms.map(u => u.binding));

  for(const uniform of astUniforms) {
    if(!config.magma) {
      state.throwError("Uniforms are only allowed when compiling for magma (pass '--magma' on the command line)!", uniform);
    }
    if(typeof uniform.binding === 'number') {
      if(uniform.binding < 0 || uniform.binding >= 2**32) {
        state.throwError("Uniform binding number must be in [0, 2^32)!", uniform);
      }
      curBindingNumber = uniform.binding;
    } else {
      while (usedBindingNumbers.has(curBindingNumber)) curBindingNumber++;
      uniform.binding = curBindingNumber;
    }
    usedBindingNumbers.add(curBindingNumber);
  }

  return astUniforms;
}

/**
 * @param {AST} ast
 * @param {RSPLConfig} config
 * @returns {ASTAttribute[]}
 */
export function astNormalizeAttributes(ast, config)
{
  const astAttributes = ast.attributes;

  

  return astAttributes;
}

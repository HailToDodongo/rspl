/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {
  nextReg, nextVecReg,
  REGS_ALLOC_SCALAR,
  REGS_ALLOC_VECTOR,
  REGS_FORBIDDEN,
  REGS_SCALAR,
  REGS_VECTOR
} from "./syntax/registers.js";
import {isVecType, isTwoRegType, TYPE_SIZE, SCALAR_TYPES, VEC_CASTS} from "./dataTypes/dataTypes.js";

const state =
{
  nextLabelId: 0,
  func: "",
  funcType: "",
  line: 0,
  outWarn: "",

  /** @type {ScopeStack} */
  scopeStack: [], // function & block scope (variables)

  /** @type {Record<string, MemVarDef>} */
  memVarMap: {}, // global variables, which are actually constants

  /** @type {Record<string, FuncDef>} */
  funcMap: {}, // function names to function objects

  reset() {
    state.nextLabelId = 0;
    state.func = "";
    state.funcType = "";
    state.line = 0;
    state.scopeStack = [];
    state.memVarMap = {};
    state.outWarn = "";
    state.outInfo = "";
    state.funcMap = {};
  },

  /**
   * @param {string} message
   * @param {any} context
   * @throws {Error}
   * @returns {never}
   */
  throwError: (message, context = {}) => {
    const lineStr = state.line === 0 ? "(???)" : state.line+"";
    const funcStr = state.func === "" ? "(???)" : state.func+"";
    throw new Error(`Error in ${funcStr}, line ${lineStr}: ${message}\n  -> AST: ${JSON.stringify(context)}`);
  },

  logWarning: (message, context) => {
    const lineStr = state.line === 0 ? "(???)" : state.line+"";
    const funcStr = state.func === "" ? "(???)" : state.func+"";
    state.outWarn += `Warning in ${funcStr}, line ${lineStr}: ${message}\n  -> AST: ${JSON.stringify(context)}\n`;
  },

  logInfo: (message) => {
    state.outInfo += message + '\n';
  },

  /**
   * Declare function in the global scope.
   * @param {string} name
   * @param {FuncArg[]} args
   */
  declareFunction: (name, args) => {
    state.funcMap[name] = {name, args};
  },

  enterFunction: (name, funcType) => {
    state.func = name;
    state.funcType = funcType;
    state.line = 0;
    state.scopeStack = [];
    state.pushScope();
  },

  leaveFunction: () => {
    state.func = "";
    state.funcType = "";
    state.line = 0;
    state.scopeStack = [];
  },

  getScope() {
    return state.scopeStack[state.scopeStack.length - 1];
  },

  /**
   * Push new scope to the scope stack.
   * This can a function, if-else, loop or manual scope.
   * @param {?string} labelStart
   * @param {?string} labelEnd
   */
  pushScope(labelStart = undefined, labelEnd = undefined)
  {
    const currScope = state.getScope();
    labelStart = labelStart || (currScope ? currScope.labelStart : undefined);
    labelEnd = labelEnd || (currScope ? currScope.labelEnd : undefined);

    state.scopeStack.push({
      varMap   : currScope ? {...currScope.varMap} : {},
      regVarMap: currScope ? {...currScope.regVarMap} : {},
      varAliasMap: currScope ? {...currScope.varAliasMap} : {},
      labelStart,
      labelEnd,
    });
    return undefined;
  },

  popScope() {
    state.scopeStack.pop();
    return undefined;
  },

  generateLabel: () => {
    ++state.nextLabelId;
    return `LABEL_${state.nextLabelId.toString(16).toUpperCase().padStart(4, '0')}`;
  },

  /**
   * Allocate register in current scope, throws if no register is available.
   * @param {DataType} type data type
   * @returns {string}
   */
  allocRegister(type) {
    // avoid collisions, this assumes a command to be the main code path, and 1 level deep calls
    const reverse = state.funcType === "command";
    const scope = state.getScope();
    let regList = isVecType(type) ? REGS_ALLOC_VECTOR : REGS_ALLOC_SCALAR;
    if(reverse)regList = [...regList].reverse();

    const twoRegs = isTwoRegType(type);
    for(const reg of regList) {
      const regNext = nextReg(reg);
      if(scope.regVarMap[reg])continue;
      if(twoRegs && (!regList.includes(regNext) || scope.regVarMap[regNext]))continue;
      return reg;
    }
    state.throwError("Out of free registers!");
  },

  /**
   * Declare variable in current scope.
   * @param {string} name
   * @param {DataType} type
   * @param {string} reg
   * @param {boolean} isConst
   */
  declareVar: (name, type, reg, isConst = false) => {
    if(name.includes(":")) {
      state.throwError("Variable name cannot contain a cast (':')!", {name});
    }
    const scope = state.getScope();
    if(!reg)state.throwError("Cannot declare variable without register!", {name});
    if(REGS_FORBIDDEN.includes(reg)) {
      state.throwError(`Cannot use reserved register '${reg}' for a variable!`, {name});
    }

    if(isVecType(type)) {
      if(!REGS_VECTOR.includes(reg))state.throwError("Cannot use scalar register for vector variable!", {name});
    } else {
      if(!REGS_SCALAR.includes(reg))state.throwError("Cannot use vector register for scalar variable!", {name});
    }

    const allocRegs = isTwoRegType(type) ? [reg, nextReg(reg)] : [reg];
    for(const allocReg of allocRegs) {
      if(scope.regVarMap[allocReg]) {
        state.throwError(`Register '${allocReg}' already used for variable '${scope.regVarMap[allocReg]}'!`, {name});
      }
    }
    scope.varMap[name] = {reg: allocRegs[0], type, isConst, modifyCount: 0};
    for(const allocReg of allocRegs) {
      scope.regVarMap[allocReg] = name;
    }
  },

  /**
   * Declare variable alias (used for macro calls) in the current scope.
   * @param {string} aliasName
   * @param {string} varName
   */
  declareVarAlias(aliasName, varName) {
    state.getRequiredVar(varName, "alias"); // check if varName exists
    const scope = state.getScope();
    const realName = scope.varAliasMap[varName] || varName; // allow alias->alias
    scope.varAliasMap[aliasName] = realName;
  },

  /**
   *
   * @param {string} name
   * @param {string} type
   * @param {number} arraySize
   */
  declareMemVar: (name, type, arraySize) => {
    state.memVarMap[name] = {name, type, arraySize};
  },

  /**
   * Fetch variable from scope, throw if undeclared.
   * @param name {string} variable name
   * @param {string} contextName context (only for logging)
   * @param {any} context (only for logging)
   * @returns {VarRegDef}
   */
  getRequiredVar: (name, contextName, context = {}) => {
    const scope = state.getScope();
    let [nameNorm, castType] = /** @type {[string, CastType]} */ name.split(":");
    nameNorm = scope.varAliasMap[nameNorm] || nameNorm;
    const res = structuredClone(scope.varMap[nameNorm]);
    if(!res)state.throwError(contextName + " Variable "+nameNorm+" not known!", context);

    // Types cast will create a "fake" variable by changing the type and register if needed.
    // For scalar types, only the type changes.
    // For vectors, they are forced to a vec16, and moved to the next register if it was a vec32+fraction cast.
    // To handle special cases, the cast is preserved (mainly used for fractional vectors).
    if(castType) {
      res.castType = castType;
      if(isVecType(res.type)) {
        if(!VEC_CASTS.includes(castType)) {
          state.throwError("Invalid cast type '"+castType+"' for variable "+nameNorm+", expected '"+VEC_CASTS.join(", ")+"'!", context);
        }
        if(res.type === "vec32" && (castType === "sfract" || castType === "ufract")) {
          res.reg = nextVecReg(res.reg);
        }
        res.type = "vec16";

      } else {
        if(!SCALAR_TYPES.includes(castType)) {
          state.throwError("Invalid cast type '"+castType+"' for variable "+nameNorm+", expected: "+SCALAR_TYPES.join(", ")+"!", context);
        }
        res.type = castType;
      }
    }

    return res;
  },

  /**
   * Marks that a variable has been modified.
   * @param {string} name
   */
  markVarModified: (name) => {
    const scope = state.getScope();
    let [nameNorm] = /** @type {[string, CastType]} */ name.split(":");
    nameNorm = scope.varAliasMap[nameNorm] || nameNorm;
    const varDef = scope.varMap[nameNorm];
    if(!varDef)state.throwError("Variable "+name+" not known!");
    varDef.modifyCount++;
  },

  /**
   * Fetch memory variable from global scope, throw if undeclared.
   * @param {string} name
   * @param {string} contextName context (only for logging)
   * @param {any} context (only for logging)
   * @returns {MemVarDef}
   */
  getRequiredMem: (name, contextName, context = {}) => {
    const res = structuredClone(state.memVarMap[name]);
    if(!res)state.throwError(contextName + " Memory-Var "+name+" not known!", context);
    return res;
  },

  /**
   * Fetches a variable or memory variable from scope, throw if undeclared.
   * @param {string} name
   * @param {string} contextName context (only for logging)
   * @param {any} context (only for logging)
   * @returns {VarRegDef|MemVarDef}
   */
  getRequiredVarOrMem: (name, contextName, context = {}) => {
    const scope = state.getScope();
    name = scope.varAliasMap[name] || name;
    let res = structuredClone(scope.varMap[name]) ||structuredClone(state.memVarMap[name]);
    if(!res) {
      state.throwError(contextName + " Variable/Memory "+name+" not known!", context);
    }
    return res;
  },

  /**
   * Fetch function from global scope, throw if undeclared.
   * @param {string} name
   * @param {any} context (only for logging)
   * @returns {FuncDef}
   */
  getRequiredFunction: (name, context = {}) => {
    const res = structuredClone(state.funcMap[name]);
    if(!res)state.throwError("Function "+name+" not known!", context);
    return res;
  }
};

export default state;
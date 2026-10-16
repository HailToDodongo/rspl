/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {
  LABELS, isVecReg,
  nextReg, nextVecReg, REG,
  REGS_ALLOC_SCALAR,
  REGS_ALLOC_VECTOR,
  REGS_FORBIDDEN,
  REGS_SCALAR,
  REGS_VECTOR
} from "./syntax/registers.js";
import {isVecType, isTwoRegType, TYPE_SIZE, SCALAR_TYPES, VEC_CASTS} from "./dataTypes/dataTypes.js";
import {validateAnnotation} from "./syntax/annotations.js";

const state =
{
  nextLabelId: 0,
  func: "",
  funcType: "",
  argSize: 0,
  line: 0,
  outWarn: "",
  sourceLines: [],    // trimmed, for the debug comments in the output
  sourceLinesRaw: [], // as written, for error context
  sourceOrigins: [],  // per preprocessed line: {file, line} it came from
  globalVars: [],     // global reserved registers / variables

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
    state.argSize = 0;
    state.line = 0;
    state.scopeStack = [];
    state.memVarMap = {};
    state.outWarn = "";
    state.outInfo = "";
    state.funcMap = {};
    state.globalVars = [];
    state.barrierMaskMap = {};
    state.barrierBitMap = {};
    state.regAllocAllowed = true;
    state.explicitRegStack = []; // regs directly asked for
    // regs the allocator had to hand out although they were wanted later
    state.fallbackAllocRegs = new Set();

    for(let globalLabel of Object.values(LABELS)) {
      this.declareMemVar(globalLabel, "u16", 1);
    }
  },

  describeLine: (lineNo) => {
    if(!lineNo)return "(???)";
    const loc = state.sourceOrigins[lineNo - 1];
    if(!loc)return lineNo + "";
    return loc.line + (loc.file ? ` (${loc.file})` : "");
  },

  /**
   * error line with one line of context on both side, e.g.
   *     11 |   vec32<$v05> a;
   *  >  12 |   vec16<$v05> b;
   *     13 | }
   * @param {number} lineNo preprocessed line number
   * @return {string}
   */
  sourceContext: (lineNo) => {
    const lines = state.sourceLinesRaw || [];
    if(!lineNo || !lines.length || lineNo > lines.length)return "";
    const shownNo = n => state.sourceOrigins[n - 1]?.line || n;
    const from = Math.max(1, lineNo - 1), to = Math.min(lines.length, lineNo + 1);
    let width = 1;
    for(let n = from; n <= to; ++n)width = Math.max(width, (shownNo(n) + "").length);
    let out = "";
    for(let n = from; n <= to; ++n) {
      out += `\n ${n === lineNo ? ">" : " "} ${(shownNo(n) + "").padStart(width, " ")} | ${lines[n - 1]}`;
    }
    return out;
  },

  
  /**
   * @param {string} message
   * @param {any} context
   * @throws {Error}
   * @returns {never}
   */
  throwError: (message, context = {}) => {
    const funcStr = state.func === "" ? "(???)" : state.func+"";
    throw new Error(`Error in ${funcStr}, line ${state.describeLine(state.line)}: ${message}${state.sourceContext(state.line)}\n  -> AST: ${JSON.stringify(context)}`);
  },

  logWarning: (message, context) => {
    const funcStr = state.func === "" ? "(???)" : state.func+"";
    state.outWarn += `Warning in ${funcStr}, line ${state.describeLine(state.line)}: ${message}${state.sourceContext(state.line)}\n  -> AST: ${JSON.stringify(context)}\n`;
  },

  declareGlobalVar: (name, type, reg, isConst = false, regFract = undefined) => {
    state.globalVars.push({name, type, reg, isConst, regFract});
  },

  logInfo: (message) => {
    state.outInfo += message + '\n';
  },

  /**
   * Declare function in the global scope.
   * @param {string} name
   * @param {FuncArg[]} args
   * @param {boolean} isRelative
   */
  declareFunction: (name, args, isRelative = false) => {
    state.funcMap[name] = {name, args, isRelative};
  },

  /**
   * @param {string} name
   * @param {string} funcType
   * @param {number} argSize
   */
  enterFunction: (name, funcType, argSize) => {
    state.func = name;
    state.funcType = funcType;
    state.argSize = argSize || 0;
    state.line = 0;
    state.scopeStack = [];
    state.pushScope();

    state.declareVar("ZERO", "u32", REG.ZERO, true);
    state.declareVar("VZERO", "vec16", REG.VZERO, true);
    state.declareVar("VSHIFT", "vec16", REG.VSHIFT, true);
    state.declareVar("VSHIFT8", "vec16", REG.VSHIFT8, true);
    state.declareVar("RA", "u32", REG.RA, false);
    state.declareVar("GP", "u32", REG.GP, false, true);
    state.declareVar("VTEMP", "vec16", REG.VTEMP0, false, true);

    // Global register variables live in every functions root scope
    for(const g of state.globalVars) {
      state.declareVar(g.name, g.type, g.reg, g.isConst, false, g.regFract);
      const def = state.getScope().varMap[g.name];
      def.isGlobal = true;
      if(g.isConst)def.modifyCount = 1;
    }
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
      annotations: currScope ? [...currScope.annotations] : [],
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
    return `LABEL_${state.func}_${state.nextLabelId.toString(16).toUpperCase().padStart(4, '0')}`;
  },

  /**
   * Puts an annotation in the current scope.
   * @param {string} name
   * @param {string|number} value
   */
  addAnnotation: (name, value, mode = "") => {
    const anno = {name, value, mode};
    validateAnnotation(anno);
    const scope = state.getScope();
    scope.annotations.push(anno);
  },

  /**
   * @return {[{name: string, value: string|number}]}
   */
  getAnnotations: (name = undefined) => {
    const scope = state.getScope();
    if(!scope)return [];

    if(name) {
      return scope.annotations.filter(a => a.name === name);
    }
    return [...scope.annotations];
  },

  clearAnnotations: () => {
    const scope = state.getScope();
    scope.annotations = [];
  },

  /**
   * @param {string} name
   * @return {number} bitmask for the given barrier
   */
  getBarrierMask: (name) =>
  {
    // we want to convert a barrier name into a unique bitflag.
    // unknown barriers are implicitly given a new bitflag.
    if(!state.barrierMaskMap[name]) {
      const len = Object.keys(state.barrierMaskMap).length;
      if(len >= 32) {
        state.throwError("Too many different barriers, only up to 32 are supported!");
      }
      state.barrierMaskMap[name] = (1 << len) >>> 0;
    }
    return state.barrierMaskMap[name];
  },

  /**
   * Ordering bit for @Barrier types (before/after/strict). 
   * each barrier tag gets a  bit above the real register space in the reorder masks
   * @param {string} name
   * @return {number} bit index relative to the end of the register space
   */
  getBarrierBit: (name) =>
  {
    if(state.barrierBitMap[name] === undefined) {
      const len = Object.keys(state.barrierBitMap).length;
      if(len >= 64) {
        state.throwError("Too many different barriers, only up to 64 are supported!");
      }
      state.barrierBitMap[name] = len;
    }
    return state.barrierBitMap[name];
  },

  /**
   * Allocate register(s) in the current scope, throws if none is available.
   * Two-register types get an adjacent pair when one is free. 
   * otherwise any two free registers
   * @param {DataType} type data type
   * @returns {{reg: string, regFract: string|undefined}}
   */
  allocRegisters(type) {
    // avoid collisions, this assumes a command to be the main code path, and 1 level deep calls
    if(!state.regAllocAllowed)state.throwError("Register allocation not allowed in this function!");

    const reverse = state.funcType === "command";
    const scope = state.getScope();
    const regListBase = isVecType(type) ? REGS_ALLOC_VECTOR : REGS_ALLOC_SCALAR;
    const regList = reverse ? [...regListBase].reverse() : regListBase;
    const twoRegs = isTwoRegType(type);

    const wanted = state.explicitRegStack[state.explicitRegStack.length - 1] || {};
    const wantedLater = reg => wanted[reg] !== undefined && wanted[reg] > state.line;
    let avoidExplicit = true;
    const isFree = reg => !scope.regVarMap[reg] && !(avoidExplicit && wantedLater(reg));
    const take = (pass, res) => {
      if(pass === 1) {
        state.fallbackAllocRegs.add(res.reg);
        if(res.regFract)state.fallbackAllocRegs.add(res.regFract);
      }
      return res;
    };

    // single register, or an adjacent pair
    for(let pass = 0; pass < 2; ++pass) {
      avoidExplicit = (pass === 0);
      for(const reg of regList) {
        if(!isFree(reg))continue;
        if(!twoRegs)return take(pass, {reg, regFract: undefined});
        const regNext = nextReg(reg);
        if(!regNext || !regListBase.includes(regNext) || !isFree(regNext))continue;
        return take(pass, {reg, regFract: regNext});
      }
    }
    // no adjacent pair left: any two free registers
    if(twoRegs) {
      for(let pass = 0; pass < 2; ++pass) {
        avoidExplicit = (pass === 0);
        let first = undefined;
        for(const reg of regList) {
          if(!isFree(reg))continue;
          if(first === undefined) { first = reg; continue; }
          return take(pass, {reg: first, regFract: reg});
        }
      }
    }

    const used = regListBase.filter(r => scope.regVarMap[r]).map(r => r + "=" + scope.regVarMap[r]).join(" ");
    state.throwError("Out of free registers! Used: " + used, regList);
  },

  /**
   * Allocate a single register in the current scope (first half for vec32).
   * @param {DataType} type data type
   * @returns {string}
   */
  allocRegister(type) {
    return state.allocRegisters(type).reg;
  },

  /**
   * Declare variable in current scope.
   * @param {string} name
   * @param {DataType} type
   * @param {string} reg
   * @param {boolean} isConst
   * @param {boolean} ignoreReserved
   * @param {string|undefined} regFract vec32 only: second register of the pair (default: the next one)
   * @param {boolean} ownsReg false for an alias(): the register stays owned by its variable
   * @param {boolean} ownsFract same for the second half
   */
  declareVar: (name, type, reg, isConst = false, ignoreReserved = false, regFract = undefined, ownsReg = true, ownsFract = true) => {
    if(name.includes(":")) {
      state.throwError("Variable name cannot contain a cast (':')!", {name});
    }
    const scope = state.getScope();
    if(!reg)state.throwError("Cannot declare variable without register!", {name});
    if(!ignoreReserved && REGS_FORBIDDEN.includes(reg)) {
      state.throwError(`Cannot use reserved register '${reg}' for a variable!`, {name});
    }

    if(isVecType(type)) {
      if(!REGS_VECTOR.includes(reg))state.throwError("Cannot use scalar register for vector variable!", {name});
    } else {
      if(!REGS_SCALAR.includes(reg))state.throwError("Cannot use vector register for scalar variable!", {name});
    }

    const checkReg = (r) => {
      const other = scope.regVarMap[r];
      if(!other)return;
      let extra = "";
      if(state.fallbackAllocRegs.has(r)) {
        extra = `\n  -> '${other}' was auto-allocated to ${r} because every other register was already taken; this function is out of ${isVecReg(r) ? "vector" : "scalar"} registers. Free one up, or give '${other}' a register of its own.`;
      }
      state.throwError(`Register '${r}' already used for variable '${other}'!` + extra, {name});
    };
    if(ownsReg)checkReg(reg);

    // Resolve the register pair once, here. Every later use reads the stored
    // pair instead of deriving the second register.
    let rFract = undefined;
    if(isTwoRegType(type)) {
      rFract = regFract || nextReg(reg);
      if(!rFract)state.throwError("No next register for two-reg type!", {name});
      if(rFract === reg)state.throwError(`A vec32 needs two different registers, '${reg}' given twice!`, {name});
      if(!isVecReg(rFract))state.throwError(`'${rFract}' is not a vector register!`, {name});
    } else if(regFract) {
      state.throwError("Only vec32 variables can specify two registers!", {name});
    }

    scope.varMap[name] = {reg, regFract: rFract, ownsReg, ownsFract, type, isConst, modifyCount: 0};
    // Borrowed registers stay owned by the variable they came from, so they
    // are neither claimed here nor freed by undef.
    if(ownsReg)scope.regVarMap[reg] = name;
    if(rFract && ownsFract) {
      checkReg(rFract);
      scope.regVarMap[rFract] = name;
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
   * Undefines a variable and all its aliases.
   * @param varName
   */
  undefVar(varName) {
    const scope = state.getScope();

    delete scope.varAliasMap[varName];
    for(let i of Object.keys(scope.varAliasMap)) {
      if(scope.varAliasMap[i] === varName) {
        delete scope.varAliasMap[i];
      }
    }

    varName = scope.varAliasMap[varName] || varName;
    const varDef = scope.varMap[varName];
    if(!varDef)state.throwError("Variable "+varName+" not known!");
    if(varDef.isGlobal)state.throwError(`Cannot undef global register variable '${varName}'!`);

    // Refuse while something still aliases one of our registers, otherwise the
    // register would go back to the allocator with the alias still pointing at it.
    const mine = [];
    if(varDef.ownsReg !== false)mine.push(varDef.reg);
    if(varDef.ownsFract !== false && varDef.regFract)mine.push(varDef.regFract);
    for(const [otherName, other] of Object.entries(scope.varMap)) {
      if(otherName === varName)continue;
      if((other.ownsReg === false && mine.includes(other.reg)) ||
         (other.ownsFract === false && mine.includes(other.regFract))) {
        state.throwError(`Cannot undef '${varName}' while '${otherName}' still aliases one of its registers!`);
      }
    }

    // Free registers (borrowed ones belong to another variable)
    for(const allocReg of mine) {
      delete scope.regVarMap[allocReg];
    }
    delete scope.varMap[varName];
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
      res.originalType = res.type;

      if(isVecType(res.type)) {
        if(!VEC_CASTS.includes(castType)) {
          state.throwError("Invalid cast type '"+castType+"' for variable "+nameNorm+", expected '"+VEC_CASTS.join(", ")+"'!", context);
        }
        if(res.type === "vec32" && (castType === "sfract" || castType === "ufract")) {
          res.reg = res.regFract || nextVecReg(res.reg);
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
   * Gets the main register of a variable, no exceptions are thrown.
   * @param name variable name
   * @return {string|undefined} register name, empty if not found
   */
  getVarReg: (name) => {
    const scope = state.getScope();
    let [nameNorm] = /** @type {[string, CastType]} */ name.split(":");
    nameNorm = scope.varAliasMap[nameNorm] || nameNorm;
    const res = scope.varMap[nameNorm];
    return res?.reg || undefined;
  },

  /**
   * Checks if a variable exists in the current scope.
   * @param name variable name
   * @return {boolean}
   */
  varExists: (name) => {
    const scope = state.getScope();
    let [nameNorm] = /** @type {[string, CastType]} */ name.split(":");
    nameNorm = scope.varAliasMap[nameNorm] || nameNorm;
    return !!scope.varMap[nameNorm];
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
   * @returns {?FuncDef}
   */
  getFunction: (name, context = {}) => {
    return state.funcMap[name] ? structuredClone(state.funcMap[name]) : undefined;
    //if(!res)state.throwError("Function "+name+" not known!", context);
  }
};

export default state;
/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

const state =
{
  nextLabelId: 0,
  func: "",
  line: 0,

  scopeStack: [], // function & block scope (variables)
  memVarMap: {}, // global variables, which are actually constants

  reset() {
    state.nextLabelId = 0;
    state.func = "";
    state.line = 0;
    state.scopeStack = [];
    state.memVarMap = {};
  },

  throwError: (message, context) => {
    const lineStr = state.line === 0 ? "(???)" : state.line+"";
    const funcStr = state.func === "" ? "(???)" : state.func+"";
    throw new Error(`Error in ${funcStr}, line ${lineStr}: ${message}\n  -> AST: ${JSON.stringify(context)}`);
  },

  enterFunction: (name) => {
    state.func = name;
    state.line = 0;
    state.scopeStack = [];
    state.pushScope();
  },

  getScope() {
    return state.scopeStack[state.scopeStack.length - 1];
  },

  pushScope() {
    const currScope = state.getScope();
    state.scopeStack.push({
      varMap   : currScope ? {...currScope.varMap} : {},
      regVarMap: currScope ? {...currScope.regVarMap} : {},
    });
  },

  popScope() {
    state.scopeStack.pop();
  },

  generateLocalLabel: () => {
    return ++state.nextLabelId;
  },

  declareVar: (name, type, reg) => {
    const currScope = state.getScope();
    // @TODO: check for conflicts
    currScope.varMap[name] = {reg, type};
    currScope.regVarMap[reg] = name;
  },

  declareMemVar: (name, type) => {
    state.memVarMap[name] = {name, type};
  },

  getRequiredVar: (name, contextName, context = {}) => {
    const currScope = state.getScope();
    const res = structuredClone(currScope.varMap[name]);
    if(!res)state.throwError(contextName + " Variable "+name+" not known!", context);
    return res;
  },

  getRequiredMem: (name, contextName, context = {}) => {
    const res = structuredClone(state.memVarMap[name]);
    if(!res)state.throwError(contextName + " Memory-Var "+name+" not known!", context);
    return res;
  },

  getRequiredVarOrMem: (name, contextName, context = {}) => {
    const currScope = state.getScope();
    let res = structuredClone(currScope.varMap[name]) ||structuredClone(state.memVarMap[name]);
    if(!res) {
      state.throwError(contextName + " Variable/Memory "+name+" not known!", context);
    }
    return res;
  }
};

export default state;
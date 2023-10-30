/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {REG} from "./syntax/registers.js";

const state =
{
  nextLabelId: 0,
  func: "",
  line: 0,
  outWarn: "",

  scopeStack: [], // function & block scope (variables)
  memVarMap: {}, // global variables, which are actually constants
  funcMap: {}, // function names to function objects

  reset() {
    state.nextLabelId = 0;
    state.func = "";
    state.line = 0;
    state.scopeStack = [];
    state.memVarMap = {};
    state.outWarn = "";
    state.funcMap = {};
  },

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

  declareFunction: (name, args) => {
    state.funcMap[name] = {name, args};
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
    if(reg === REG.VDUMMY)state.logWarning("Using $v27 (VDUMMY) for a variable, might get overwritten by certain operations!", {name});
    if(reg === REG.VTEMP)state.throwError("Cannot use $v28 (VTEMP) for a variable!", {name});
    if(reg === REG.VTEMP2)state.throwError("Cannot use $v29 (VTEMP2) for a variable!", {name});
    if(reg === REG.AT)state.throwError("Cannot use $at (AT) for a variable!", {name});

    currScope.varMap[name] = {reg, type};
    currScope.regVarMap[reg] = name;
  },

  declareMemVar: (name, type, arraySize) => {
    state.memVarMap[name] = {name, type, arraySize};
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
  },

  getRequiredFunction: (name, context = {}) => {
    const res = structuredClone(state.funcMap[name]);
    if(!res)state.throwError("Function "+name+" not known!", context);
    return res;
  }
};

export default state;
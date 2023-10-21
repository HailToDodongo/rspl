/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

const state =
{
  func: "",
  line: "",

  varMap: {},
  regVarMap: {},

  throwError: (message, context) => {
    const lineStr = state.line === 0 ? "(???)" : state.line+"";
    const funcStr = state.func === "" ? "(???)" : state.func+"";
    throw new Error(`Error in ${funcStr}, line ${lineStr}: ${message}\n  -> AST: ${JSON.stringify(context)}`);
  },

  enterFunction: (name) => {
    state.func = name;
    state.varMap = {};
    state.regVarMap = {};
  },

  declareVar: (name, type, reg) => {
    // @TODO: check for conflicts
    state.varMap[name] = {reg, type};
    state.regVarMap[reg] = name;
  },

  getRequiredVar: (name, contextName, context = {}) => {
    const res = structuredClone(state.varMap[name]);
    if(!res)state.throwError(contextName + " Variable "+name+" not known!", context);
    return res;
  }
};

export default state;
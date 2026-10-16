/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import opsScalar from "./operations/scalar";
import opsVector from "./operations/vector";
import state from "./state";
import builtins from "./builtins/functions";
import {isVecReg, REG, REGS_FORBIDDEN, nextReg} from "./syntax/registers.js";
import {asm, asmBranch, asmLabel, asmNOP} from "./intsructions/asmWriter.js";
import {opBranch} from "./operations/branch.js";
import {callUserFunction} from "./operations/userFunction.js";
import {isVecType, isTwoRegType} from "./dataTypes/dataTypes.js";
import {POW2_SWIZZLE_VAR} from "./syntax/swizzle.js";
import {LABEL_CMD_LOOP} from "./builtins/libdragon.js";
import scalar from "./operations/scalar";
import {ANNOTATIONS, getAnnotationVal} from "./syntax/annotations.js";
import {astCalcPartsToASM} from "./astCalcNormalizer.js";

const VECTOR_TYPES = ["vec16", "vec32"];

/**
 * @param {ASTCalc} calc
 * @param {ASTFuncArg} varRes
 * @returns {ASM[]}
 */
function calcToAsm(calc, varRes)
{
  switch(calc.type)
  {
    case "calcVar": {
      const varRight = state.getRequiredVar(calc.right.value, "right", calc);
      varRight.swizzle = calc.swizzleRight;
      return calcAssignToAsm(calc, varRes, varRight);
    }

    case "calcNum": {
      const varRight = {type: varRes.type, value: calc.right.value};
      return calcAssignToAsm(calc, varRes, varRight);
    }

    case 'calcLR': {
        const typeL = calc.left.type;
        const typeR = calc.right.type;
        if(typeL === "VarName" && typeR === "VarName")
        {
          const varLeft = state.getRequiredVar(calc.left.value, "Left", calc);
          const varRight = state.getRequiredVar(calc.right.value, "right", calc);
          varLeft.swizzle = calc.swizzleLeft;
          varRight.swizzle = calc.swizzleRight;
          return calcLRToAsm(calc, varRes, varLeft, varRight);
        }
        if(typeL === "VarName" && typeR === "num")
        {
          const varLeft = state.getRequiredVar(calc.left.value, "Left", calc);
          varLeft.swizzle = calc.swizzleLeft;
          return calcLRToAsm(calc, varRes, varLeft, {type: varLeft.type, value: calc.right.value});
        }
        state.throwError("Unimplemented calcLR type: " + typeL + " " + typeR, calc);
    } break;

    case "calcFunc": {
      const builtinFunc = builtins[calc.funcName];
      if(!builtinFunc)state.throwError("Unknown builtin: " + calc.funcName, calc);
      return builtinFunc(varRes, calc.args, calc.swizzleRight);
    }

    case "calcCompare": {
      const varLeft = state.getRequiredVar(calc.left, "left", calc);
      if(!isVecType(varRes.type)) {
        /** @type {ASTFuncArg} */
        let varRight;
        if(calc.right.type === "num") {
          if(calc.right.value === 0) {
            varRight = {type: varLeft.type, reg: REG.ZERO};
          } else {
            varRight = {type: varLeft.type, reg: REG.AT};
            scalar.loadImmediate(varRight.reg, calc.right.value);
          }
        } else {
          varRight = state.getRequiredVar(calc.right.value, "right", calc);
        }

        return opsScalar.opCompare(varRes, varLeft, varRight, calc.op, calc.ternary);
      }
      /** @type {ASTFuncArg} */
      let varRight;

      // right side of a compare can be a 2^x constant, resolve this back into a var
      if(calc.right.type === "num") {
        varRight = POW2_SWIZZLE_VAR[calc.right.value + ""];
        if(!varRight)state.throwError("Constant must be a power of two! " + calc.right.value, calc);
      } else {
        varRight = state.getRequiredVar(calc.right.value, "right", calc);
        varRight.swizzle = calc.swizzleRight;
      }
      return opsVector.opCompare(varRes, varLeft, varRight, calc.op, calc.ternary);
    }

    default: state.throwError("Unknown calculation type: " + calc.type, calc);
  }
}

/**
 * @param {ASTCalc} calc
 * @param varRes
 * @param varRight
 * @returns {ASM[]}
 */
function calcAssignToAsm(calc, varRes, varRight) {
  const isVector = VECTOR_TYPES.includes(varRes.type);
  const opsHandler = isVector ? opsVector : opsScalar;

  if(!isVector && calc.op && (calc.swizzleLeft || calc.swizzleRight)) {
    state.throwError("Swizzling not allowed for scalar operations!", calc);
  }

  switch (calc.op) {
    case "!":  state.throwError("Unary '!'-operator not implemented!"); break;
    case "~": return opsHandler.opBitFlip(varRes, varRight);
    default: return opsHandler.opMove(varRes, varRight);
  }
}

/**
 * @param {ASTCalc} calc
 * @param {ASTFuncArg} varRes
 * @param {ASTFuncArg} varLeft
 * @param {ASTFuncArg} varRight
 * @returns {ASM[]}
 */
function calcLRToAsm(calc, varRes, varLeft, varRight)
{
  const op = calc.op;
  if(varLeft.type !== varRight.type || varLeft.type !== varRes.type) {
    //state.throwError("Type mismatch!", [varLeft.type, varRight.type, varRes.type]);
  }

  const isVector = VECTOR_TYPES.includes(varRes.type);
  const opsHandler = isVector ? opsVector : opsScalar;

  if(!isVector && (calc.swizzleLeft || calc.swizzleRight)) {
    state.throwError("Swizzling not allowed for scalar operations!");
  }

  switch (op) {
    case  "+":  return opsHandler.opAdd(varRes, varLeft, varRight);
    case  "-":  return opsHandler.opSub(varRes, varLeft, varRight);
    case  "*":  return opsHandler.opMul(varRes, varLeft, varRight, true);
    case "+*":  return opsHandler.opMul(varRes, varLeft, varRight, false);
    case  "/":  return opsHandler.opDiv(varRes, varLeft, varRight);

    case "&":  return opsHandler.opAnd(varRes, varLeft, varRight);
    case "|":  return opsHandler.opOr(varRes, varLeft, varRight);
    case "~|": return opsHandler.opNOR(varRes, varLeft, varRight);
    case "^":  return opsHandler.opXOR(varRes, varLeft, varRight);

    case "<<":  return opsHandler.opShiftLeft(varRes, varLeft, varRight);
    case ">>":  return opsHandler.opShiftRight(varRes, varLeft, varRight, false);
    case ">>>": return opsHandler.opShiftRight(varRes, varLeft, varRight, true);

    default: state.throwError("Unknown operator: " + op);
  }
}

let coldBlocks = [];

/**
 * @param {ASTIf} st
 * @returns {ASM[]}
 */
function ifToASM(st)
{
  if(st.compare.left.type === "num") {
    return state.throwError("IF-Statements with numeric left-hand-side not implemented!", st);
  }
  const varLeft = state.getRequiredVar(st.compare.left.value, "left", st);
  if(isVecReg(varLeft.reg)) {
    return state.throwError("IF-Statements must use scalar-registers!", st);
  }

  if(state.getAnnotations("Unlikely").length) {
    if(st.blockElse)state.throwError("@Unlikely if-statements cannot have an else-block!", st);

    const labelCold = state.generateLabel();
    const labelJoin = state.generateLabel();

    // invert guard: branch into the cold block when the condition is true
    const res = opBranch(st.compare, labelCold, true);
    state.clearAnnotations();
    res.push(asmLabel(labelJoin));

    state.pushScope();
    const ifBlock = scopedBlockToASM(st.blockIf);
    state.popScope();

    coldBlocks.push(asmLabel(labelCold), ...ifBlock, asm("j", [labelJoin]), asmNOP());
    return res;
  }

  const labelElse = state.generateLabel();
  const labelEnd = st.blockElse ? state.generateLabel() : labelElse;
  const res = [];

  // Branch condition
  res.push(...opBranch(st.compare, labelElse));

  // IF-Block
  state.pushScope();
  res.push(...scopedBlockToASM(st.blockIf));
  if(st.blockElse)res.push(asm("beq", [REG.ZERO, REG.ZERO, labelEnd]), asmNOP());
  state.popScope();

  // ELSE-Block
  if(st.blockElse) {
    state.pushScope(undefined, labelElse);
    res.push(asmLabel(labelElse), ...scopedBlockToASM(st.blockElse));
    state.popScope();
  }
  res.push(asmLabel(labelEnd));

  return res;
}

/**
 * @param {ASTWhile} st
 * @returns {ASM[]}
 */
function whileToASM(st)
{
  if(st.compare.left.type === "num") {
    return state.throwError("While-Statements with numeric left-hand-side not implemented!", st);
  }
  const varLeft = state.getRequiredVar(st.compare.left.value, "left", st);
  if(isVecReg(varLeft.reg)) {
    return state.throwError("While-Statements must use scalar-registers!", st);
  }

  const labelStart = state.generateLabel();
  const labelEnd = state.generateLabel();

  /***** Loop: *****
   * label_start:
   *   if(!condition)goto label_end;
   *   ...
   *   goto label_start;
   *   label_end;
   */
  return [
    asmLabel(labelStart),
    ...opBranch(st.compare, labelEnd),
    state.pushScope(labelStart, labelEnd),
      ...scopedBlockToASM(st.block),
      asm("j", [labelStart]),
      asmNOP(),
    state.popScope(),
    asmLabel(labelEnd),
  ];
}

function loopToASM(st)
{
  const labelStart = state.generateLabel();
  const labelEnd = state.generateLabel();

  if(st.compare) {
    if(st.compare.left.type === "num") {
      return state.throwError("Loop-Statements with numeric left-hand-side not implemented!", st);
    }
    const varLeft = state.getRequiredVar(st.compare.left.value, "left", st);
    if(isVecReg(varLeft.reg)) {
      return state.throwError("Loop-Statements must use scalar-registers!", st);
    }

    return [
      asmLabel(labelStart),
      state.pushScope(labelStart, labelEnd),
        ...scopedBlockToASM(st.block),
        ...opBranch(st.compare, labelStart, true), // invert condition!
      state.popScope(),
      asmLabel(labelEnd),
    ];
  }

  return [
    asmLabel(labelStart),
    state.pushScope(labelStart, labelEnd),
      ...scopedBlockToASM(st.block),
      asm("j", [labelStart]),
      asmNOP(),
    state.popScope(),
    asmLabel(labelEnd),
  ];
}

/**
 * Collect every register a block directly allocates
 */
function collectExplicitRegs(block, out = {})
{
  const note = (reg, isAlias, line) => {
    if(!reg || isAlias)return;
    if(out[reg] === undefined || out[reg] < line)out[reg] = line;
  };
  for(const st of (block && block.statements) || [])
  {
    switch(st.type) {
      case "varDecl":
      case "varDeclAssign":
        note(st.reg, st.regAlias, st.line || 0);
        note(st.regFract, st.regFractAlias, st.line || 0);
      break;
      case "varDeclMulti": {
        note(st.regFract, st.regFractAlias, st.line || 0);
        if(!st.reg || st.regAlias)break;
        const step = isTwoRegType(st.varType) ? 2 : 1;
        st.varNames.forEach((_, i) => note(nextReg(st.reg, i * step) || st.reg, false, st.line || 0));
      } break;
      case "if":
        collectExplicitRegs(st.blockIf, out);
        collectExplicitRegs(st.blockElse, out);
      break;
      case "while":
      case "loop":
        collectExplicitRegs(st.block, out);
      break;
      case "scopedBlock":
        collectExplicitRegs(st, out);
      break;
    }
  }
  return out;
}

function resolveRegSlot(spec, isAlias, declName)
{
  if(!isAlias)return {reg: spec, owned: true};
  const target = state.getRequiredVar(spec, "alias");
  if(target.type === "vec32") {
    state.throwError(`alias(${spec}) is ambiguous for a vec32, pick a half with ':sint' or ':ufract'!`, declName);
  }
  return {reg: target.reg, owned: false};
}

function scopedBlockToASM(block, args = [], isCommand = false)
{
  // registers wanted by name further down this block are kept clear by the allocator
  state.explicitRegStack.push(collectExplicitRegs(block));
  try {
    return scopedBlockToASMInner(block, args, isCommand);
  } finally {
    state.explicitRegStack.pop();
  }
}

/**
 * @param {ASTScopedBlock} block
 * @param {any[]} args
 * @param {boolean} isCommand
 * @returns {ASM[]}
 */
function scopedBlockToASMInner(block, args = [], isCommand = false)
{
  const res = [];

  let argIdx = 0;
  for(const arg of args)
  {
    let reg = arg.reg, regFract = arg.regFract;
    if(!reg) {
      if(argIdx < 4) {
        reg = "$a" + argIdx;
      } else {
        const regs = state.allocRegisters(arg.type);
        reg = regs.reg;
        if(!regFract)regFract = regs.regFract;
      }
    }
    state.declareVar(arg.name, arg.type, reg, false, false, regFract);

    if(isCommand && argIdx >= 4) {
      const offset = argIdx * 4 - args.length * 4;
      const loadOp = {u8: "lbu", s8: "lb", u16: "lhu", s16: "lh"}[arg.type] || "lw";
      res.push(asm(loadOp, [reg, `%lo(RSPQ_DMEM_BUFFER ${offset < 0 ? "" : "+"} ${offset})(${REG.GP})`]));
    }
    ++argIdx;
  }

  for(const st of block.statements)
  {
    state.line = st.line || 0;

    switch(st.type) 
    {
      case "varDecl": {
        let reg = st.reg, regFract = st.regFract;
        let ownsReg = true, ownsFract = true;
        if(reg) {
          ({reg, owned: ownsReg} = resolveRegSlot(reg, st.regAlias, st.varName));
        } else {
          const regs = state.allocRegisters(st.varType);
          reg = regs.reg;
          if(!regFract)regFract = regs.regFract;
        }
        if(regFract) {
          ({reg: regFract, owned: ownsFract} = resolveRegSlot(regFract, st.regFractAlias, st.varName));
        }
        state.declareVar(st.varName, st.varType, reg, st.isConst || false, false, regFract, ownsReg, ownsFract);
      } break;

      case "varUndef": {
        for(const name of (st.varNames || [st.varName])) {
          state.undefVar(name);
        }
      } break;

      case "varDeclAlias":
        state.declareVarAlias(st.aliasName, st.varName);
      break;

      case "varAssignCalc": {
        const calc = st.calc;
        const varRes = structuredClone(
          state.getRequiredVar(st.varName, "result", st)
        );
        if(!varRes)state.throwError("Destination Variable "+st.varName+" not known!", st);
        if(varRes.isConst && varRes.modifyCount > 0)state.throwError("Cannot assign to constant variable!", st);
        varRes.swizzle = st.swizzle;

        state.markVarModified(st.varName);
        res.push(...calcToAsm(calc, varRes));
      } break;

      case "funcCall": {
        const builtinFunc = builtins[st.func];
        if(builtinFunc) {
          res.push(...builtinFunc(undefined, st.args, undefined));
        } else {
          res.push(...callUserFunction(st.func, st.args));
        }

      } break;

      case "annotation": {
        state.addAnnotation(st.name, st.value, st.mode || "");
      } break;

      case "labelDecl":
        res.push(asmLabel(st.name));
      break;

      case "goto" : {
        // check if we jump to a label or variable
        const refJump = state.getVarReg(st.label) || st.label;
        res.push(asm("j", [refJump]), asmNOP());
      } break;
      case "if"   : res.push(...ifToASM(st));           break;
      case "while": res.push(...whileToASM(st));        break;
      case "loop" : res.push(...loopToASM(st));         break;

      case "break": {
        const labelEnd = state.getScope().labelEnd;
        if(!labelEnd) {
          //state.throwError("'break' cannot find a label to jump to!", st);
          res.push(asm("jr", [REG.RA]), asmNOP());
        } else {
          res.push(asm("j", [labelEnd]), asmNOP());
        }
      } break;

      case "exit": {
        res.push(asm("j", [LABEL_CMD_LOOP]), asmNOP());
      } break;

      case "continue": {
        const labelStart = state.getScope().labelStart;
        if(!labelStart)state.throwError("'continue' cannot find a label to jump to!", st);
        res.push(asm("j", [labelStart]), asmNOP());
      } break;

      case "scopedBlock":
        state.pushScope();
        res.push(...scopedBlockToASM(st));
        state.popScope();
      break;

      case "nestedCalc":
        const ast = astCalcPartsToASM(st);
        res.push(...scopedBlockToASM({statements: ast}));
      break;

      default:
        state.throwError("Unknown statement type: " + st.type, st);
    }

    if(st.type !== "annotation") {
      state.clearAnnotations();
    }
  }
  return res;
}

function getArgSize(block)
{
  if(block.type === "shader")return 8;
  if(block.type !== "command")return 0;
  // each arg is always 4-bytes, the first one is implicitly set
  return Math.max(block.args.length * 4, 4);
}

/**
 * @param {AST} ast
 * @return {ASMFunc[]}
 */
export function ast2asm(ast)
{
  /** @type {ASMFunc[]} */
  const res = [];

  const uniformsState = ast.uniforms.flatMap(u => u.state);

  for(const stateVar of [...ast.state, ...ast.stateData, ...ast.stateBss, ...uniformsState]) {
    const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
    state.declareMemVar(stateVar.varName, stateVar.varType, arraySize);
  }

  for(const block of ast.functions)
  {
    if(["function", "command"].includes(block.type)) {
      state.declareFunction(block.name, block.args,
        !!getAnnotationVal(block.annotations || [], ANNOTATIONS.Relative)
      );
    }
  }

  // Global register variables:
  {
    state.func = "(global)";
    // registers the built-ins
    const usedRegs = {
      [REG.ZERO]: "ZERO", [REG.VZERO]: "VZERO", [REG.VSHIFT]: "VSHIFT", [REG.VSHIFT8]: "VSHIFT8",
      [REG.RA]: "RA", [REG.GP]: "GP", [REG.VTEMP0]: "VTEMP",
    };
    const usedNames = new Set();
    for(const g of ast.globalVars || [])
    {
      state.line = g.line || 0;
      const name = g.varNames[0];
      if(!g.reg)state.throwError(`Global variable '${name}' must specify a register, e.g. '${g.varType}<$t0> ${name};'`, name);
      if(g.varNames.length > 1)state.throwError("Global variables must be declared one per statement (each needs its own register)!", name);
      if(g.hasInit)state.throwError(`Global variable '${name}' cannot have an initializer (no code runs at file scope)!`, name);
      if(g.regAlias || g.regFractAlias)state.throwError(`Global variable '${name}' cannot alias another variable!`, name);
      if(usedNames.has(name))state.throwError(`Global variable '${name}' already declared!`, name);
      usedNames.add(name);
      if(REGS_FORBIDDEN.includes(g.reg))state.throwError(`Cannot use reserved register '${g.reg}' for a global variable!`, name);
      if(isVecType(g.varType) && !isVecReg(g.reg))state.throwError("Cannot use scalar register for vector variable!", name);
      if(!isVecType(g.varType) && isVecReg(g.reg))state.throwError("Cannot use vector register for scalar variable!", name);

      const claimReg = r => {
        if(usedRegs[r])state.throwError(`Register '${r}' already used for variable '${usedRegs[r]}'!`, name);
        usedRegs[r] = name;
      };
      claimReg(g.reg);
      let regFract = g.regFract;
      if(isTwoRegType(g.varType)) {
        if(!regFract)regFract = nextReg(g.reg);
        if(!regFract)state.throwError("No next register for two-reg type!", name);
        claimReg(regFract);
      }
      state.declareGlobalVar(name, g.varType, g.reg, g.isConst, regFract);
    }
  }

  for(const block of ast.functions)
  {
    state.func = block.name || "";
    state.line = block.line || 0;

    if(["function", "command", "shader"].includes(block.type)) {

      if(!block.body)continue;
      state.enterFunction(block.name, block.type, getArgSize(block));
      state.regAllocAllowed = !getAnnotationVal(block.annotations || [], ANNOTATIONS.NoRegAlloc);

      coldBlocks = [];
      const blockAsm = scopedBlockToASM(block.body, block.args, block.type === "command");
      ++state.line;

      const needsReturn = !getAnnotationVal(block.annotations || [], ANNOTATIONS.NoReturn);

      if(needsReturn) {
        if(["command", "shader"].includes(block.type)) {
          blockAsm.push(asm("j", [LABEL_CMD_LOOP]), asmNOP());
        } else {
          blockAsm.push(asm("jr", [REG.RA]), asmNOP());
        }
      }
      // out-of-line @Unlikely blocks follow the return
      blockAsm.push(...coldBlocks);
      coldBlocks = [];

      res.push({
        ...block,
        asm: blockAsm.filter(Boolean),
        argSize: getArgSize(block),
        cyclesBefore: 0,
        cyclesAfter: 0,
        body: undefined
      });

      state.leaveFunction();
    }
  }
  return res;
}

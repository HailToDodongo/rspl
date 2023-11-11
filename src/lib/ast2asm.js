/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import opsScalar from "./operations/scalar";
import opsVector from "./operations/vector";
import state from "./state";
import builtins from "./builtins/functions";
import {isVecReg, REG} from "./syntax/registers.js";
import {asm, asmComment, asmLabel, asmNOP} from "./intsructions/asmWriter.js";
import {opBranch} from "./operations/branch.js";
import {callUserFunction} from "./operations/userFunction.js";

const VECTOR_TYPES = ["vec16", "vec32"];

/**
 * @param {ASTCalc} calc
 * @param {ASTFuncArg} varRes
 * @returns {[{args: *, op: *, type: number},({args: *, op: *, type: number}|null)]|[{args: *, op: *, type: number}]|*|[{args: *, op: *, type: number},{args: *, op: *, type: number},{args: *, op: *, type: number}]|[{comment: *, type: number}]|[{args: *, op: *, type: number},{args: *, op: *, type: number},{args: *, op: *, type: number},{args: *, op: *, type: number},{args: *, op: *, type: number}]|{args: *, op: *, type: number}[]|[{args: *, op: *, type: number},{args: *, op: *, type: number}]}
 */
function calcToAsm(calc, varRes)
{
  switch(calc.type)
  {
    case "calcVar": {
      const varRight = state.getRequiredVar(calc.right, "right", calc);
      varRight.swizzle = calc.swizzleRight;
      return calcAssignToAsm(calc, varRes, varRight);
    }

    case "calcNum": {
      const varRight = {type: varRes.type, value: calc.right};
      return calcAssignToAsm(calc, varRes, varRight);
    }

    case "calcVarVar": {
      const varLeft = state.getRequiredVar(calc.left, "Left", calc);
      const varRight = state.getRequiredVar(calc.right, "right", calc);
      varLeft.swizzle = calc.swizzleLeft;
      varRight.swizzle = calc.swizzleRight;

      return calcLRToAsm(calc, varRes, varLeft, varRight);
    }

    case "calcVarNum": {
      const varLeft = state.getRequiredVar(calc.left, "Left", calc);
      varLeft.swizzle = calc.swizzleLeft;

      return calcLRToAsm(calc, varRes, varLeft, {type: varLeft.type, value: calc.right});
    }

    case "calcFunc": {
      const builtinFunc = builtins[calc.funcName];
      if(!builtinFunc)state.throwError("Unknown builtin: " + calc.funcName, calc);
      return builtinFunc(varRes, calc.args, calc.swizzleRight);
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
    case "^":  return opsHandler.opXOR(varRes, varLeft, varRight);

    case "<<":  return opsHandler.opShiftLeft(varRes, varLeft, varRight);
    case ">>":  return opsHandler.opShiftRight(varRes, varLeft, varRight);

    default: state.throwError("Unknown operator: " + op);
  }
}

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

  const labelElse = state.generateLocalLabel();
  const labelEnd = st.blockElse ? state.generateLocalLabel() : labelElse;
  const res = [];

  // Branch condition
  res.push(...opBranch(st.compare, labelElse+"f"));

  // IF-Block
  state.pushScope();
  res.push(...scopedBlockToASM(st.blockIf));
  if(st.blockElse)res.push(asm("beq", [REG.ZERO, REG.ZERO, labelEnd+"f"]), asmNOP());
  state.popScope();

  // ELSE-Block
  if(st.blockElse) {
    state.pushScope();
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

  const labelStart = state.generateLocalLabel();
  const labelEnd = state.generateLocalLabel();

  /***** Loop: *****
   * label_start:
   *   if(!condition)goto label_end;
   *   ...
   *   goto label_start;
   *   label_end;
   */
  return [
    asmLabel(labelStart),
    ...opBranch(st.compare, labelEnd+"f"),
    state.pushScope(),
      ...scopedBlockToASM(st.block),
      asm("j", [labelStart+"b"]),
      asmNOP(),
    state.popScope(),
    asmLabel(labelEnd),
  ];
}

/**
 * @param {ASTScopedBlock} block
 * @param args
 * @returns {ASM[]}
 */
function scopedBlockToASM(block, args = [])
{
  const res = [];

  let argIdx = 0;
  for(const arg of args)
  {
    let reg = arg.reg || "$a"+argIdx;
    if(argIdx >= 4) { // args beyond that live in RAM, fetch them and expect a target register
      if(!arg.reg)state.throwError("Argument "+argIdx+" '"+arg.name+"' needs a target register!", arg);
      const totalSize = args.length * 4;
      res.push(asm("lw", [arg.reg, `%lo(RSPQ_DMEM_BUFFER) - ${totalSize - argIdx*4}(${REG.GP})`]));
    }

    state.declareVar(arg.name, arg.type, reg);
    ++argIdx;
  }

  for(const st of block.statements)
  {
    state.line = st.line || 0;

    switch(st.type) 
    {
      case "comment":
        res.push(asmComment(st.comment.substring(2).trimEnd() || ""));
      break;

      case "varDecl": {
        const reg = st.reg || state.allocRegister(st.varType);
        state.declareVar(st.varName, st.varType, reg);
      } break;

      case "varDeclAlias":
        state.declareVarAlias(st.aliasName, st.varName);
      break;

      case "varAssignCalc": {
        const calc = st.calc;
        const varRes = structuredClone(
          state.getRequiredVar(st.varName, "result", st)
        );
        varRes.swizzle = st.swizzle;
        if(!varRes)state.throwError("Destination Variable "+st.varName+" not known!", st);

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

      case "labelDecl":
        res.push(asmLabel(st.name));
      break;

      case "goto" : res.push(asm("j", [st.label]), asmNOP()); break;
      case "if"   : res.push(...ifToASM(st));           break;
      case "while": res.push(...whileToASM(st));        break;

      case "scopedBlock":
        state.pushScope();
        res.push(...scopedBlockToASM(st));
        state.popScope();
      break;

      default:
        state.throwError("Unknown statement type: " + st.type, st);
    }
  }
  return res;
}

function getArgSize(block)
{
  if(block.type !== "command")return 0;
  // each arg is always 4-bytes, the first one is implicitly set
  return Math.max(block.args.length * 4, 4);
}

export function ast2asm(ast)
{
  const res = [];

  for(const stateVar of ast.state) {
    const arraySize = stateVar.arraySize.reduce((a, b) => a * b, 1) || 1;
    state.declareMemVar(stateVar.varName, stateVar.varType, arraySize);
  }

  for(const block of ast.functions)
  {
    state.func = block.name || "";
    state.line = block.line || 0;

    if(["function", "command"].includes(block.type)) {
      state.declareFunction(block.name, block.args);

      if(!block.body)continue;
      state.enterFunction(block.name, block.type);

      const blockAsm = scopedBlockToASM(block.body, block.args);
      if(block.type === "command") {
        blockAsm.push(asm("j", ["RSPQ_Loop"]), asmNOP());
      } else {
        blockAsm.push(asm("jr", [REG.RA]), asmNOP());
      }

      res.push({
        ...block,
        asm: blockAsm.filter(Boolean),
        argSize: getArgSize(block),
        body: undefined
      });

      state.leaveFunction();
    }
  }
  return res;
}

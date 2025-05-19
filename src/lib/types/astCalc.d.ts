/**
 * NOTE: this project does NOT use TypeScript.
 * Types defined here are solely used by JSDoc.
 */
import {ASTFuncArg} from "./ast";

declare global
{
    type CalcOp = '+' | '-' | '*' | '+*' | '/' | '<<' | '>>' | '&' | '|' | '^' | '!' | '~';
    type CompareOp = '==' | '!=' | '<' | '<=' | '>' | '>=';

    type ASTCompareArg = {
      type: 'num' | 'var';
      value: number | string;
    };

    type ASTCompare = {
        type: 'compare';
        left: ASTCompareArg;
        op: CompareOp;
        right: ASTCompareArg;
        line: number;
    }

    type ASTExprNum = {
      type: 'num',
      value: number;
    };
    type ASTExprVarName = {
      type: 'VarName',
      value: string;
    };

    type ASTCalcNum = {
        type: 'calcNum';
        right: ASTExprNum;
    };

    type ASTCalcVar = {
        type: 'calcVar';
        left?: string;
        op?: CalcOp;
        right: ASTExprVarName;
        swizzleLeft?: Swizzle;
        swizzleRight?: Swizzle;
    };

    type ASTCalcLR = {
        type: 'calcLR';
        left: ASTExprNum | ASTExprVarName | ASTCalcLR;
        op: CalcOp;
        right: ASTExprNum | ASTExprVarName;
        swizzleLeft?: Swizzle;
        swizzleRight?: Swizzle;
    };

    type ASTCalcFunc = {
        type: 'calcFunc';
        funcName: string;
        args: ASTFuncArg[];
        swizzleRight?: Swizzle;
    };

    type ASTTernary = {
        left: string;
        right: string;
        swizzleRight?: Swizzle;
    };

    type ASTCalcCompare = {
        type: 'calcCompare';
        left: string;
        op: CalcOp;
        right: ASTExprNum | ASTExprVarName;
        swizzleRight?: Swizzle;
        ternary?: ASTTernary
    };

    type ASTCalc = ASTCalcLR | ASTCalcNum | ASTCalcVar | ASTCalcFunc | ASTCalcCompare;
}
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

    type ASTCalcNum = {
        type: 'calcNum';
        right: number;
    };

    type ASTCalcVarNum = {
        type: 'calcVarNum';
        left: string;
        op: CalcOp;
        right: number;
        swizzleLeft?: Swizzle;
    };

    type ASTCalcVar = {
        type: 'calcVar';
        left?: string;
        op?: CalcOp;
        right: string;
        swizzleLeft?: Swizzle;
        swizzleRight?: Swizzle;
    };

    type ASTCalcVarVar = {
        type: 'calcVarVar';
        left: string;
        op: CalcOp;
        right: string;
        swizzleLeft?: Swizzle;
        swizzleRight?: Swizzle;
    };

    type ASTCalcFunc = {
        type: 'calcFunc';
        funcName: string;
        args: ASTFuncArg[];
        swizzleRight?: Swizzle;
    };

    type ASTCalc = ASTCalcNum | ASTCalcVarNum | ASTCalcVar | ASTCalcFunc | ASTCalcVarVar;
}
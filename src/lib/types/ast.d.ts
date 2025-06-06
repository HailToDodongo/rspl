/**
 * NOTE: this project does NOT use TypeScript.
 * Types defined here are solely used by JSDoc.
 */

import './astCalc';
import './types';
import './asm';

declare global
{
    type ASTStmType = 'scopedBlock' | 'varDef' | 'varAssign' | 'funcCall' | 'return' | 'if' | 'while' | 'for' | 'break' | 'continue' | 'asm';
    type ASTFuncType = 'function' | 'macro' | 'command';

    type ASTFuncArg = {
        type: 'num' | 'var' | 'string' | DataType;
        reg?: string;
        name?: string;
        value?: number | string;
        swizzle?: Swizzle;
        castType?: CastType;
        originalType?: DataType;
    };

    type ASTFunc = {
      annotations: Annotation[];
      type: ASTFuncType;
      resultType?: number;
      name: string;
      nameOverride?: string;
      body: ASTScopedBlock;
      args: ASTFuncArg[];
    };

    type ASTMacroMap = Record<string, ASTFunc>;

    type ASTState = {
        arraySize: number[];
        extern: boolean;
        type: string;
        varName: string;
        varType: DataType;
        align: number;
        value: number[] | undefined;
    };

    type ASTStatementBase = { line: number; };

    type ASTScopedBlock = ASTStatementBase & {
        type: 'scopedBlock';
        statements: ASTStatement[];
    };

    type ASTIf = ASTStatementBase & {
        type: 'if';
        blockIf: ASTScopedBlock;
        blockElse?: ASTScopedBlock;
        compare: ASTCompare;
    };

    type ASTWhile = ASTStatementBase & {
        type: 'while';
        block: ASTScopedBlock;
        compare: ASTCompare;
    };

    type ASTLoop = ASTStatementBase & {
        type: 'loop';
        block: ASTScopedBlock;
        compare?: ASTCompare;
    };

    type ASTDeclAssign = ASTStatementBase & {
        type: 'varDeclAssign';
        varName: string;
        calc: ASTCalc;
        isConst: boolean;
    };

    type ASTDeclMulti = ASTStatementBase & {
        type: 'varDeclMulti';
        varNames: string[];
        reg: string;
        varType: DataType;
        isConst: boolean;
    };

    type ASTDecl = ASTStatementBase & {
        type: 'varDecl';
        varName: string;
        reg: string;
        varType: DataType;
        isConst: boolean;
    };

    type ASTDeclAlias = ASTStatementBase & {
        type: 'varDeclAlias';
        varName: string;
        aliasName: string;
    };

    type ASTNestedCalcPart = string | ASTNestedCalcPart[] | Object;

    type ASTNestedCalc = ASTStatementBase & {
      type: 'nestedCalc';
      varName: string;
      swizzle?: Swizzle;
      parts: ASTNestedCalcPart[];
    };

    type ASTAssignCalc = ASTStatementBase & {
        type: 'varAssignCalc';
        calc: ASTCalc;
        swizzle?: Swizzle;
        varName: string;
        assignType: "=" | "&&=" | "||=" | "&=" | "|=" | "<<=" | ">>=" | "+*=" | "+=" | "-=" | "*=" | "/=",
    };

    type ASTFuncCall = ASTStatementBase & {
        type: 'funcCall';
        func: any;
        args: ASTFuncArg[];
    };

    type ASTLabelDecl = ASTStatementBase & {
        type: 'labelDecl';
        name: string;
    };

    type ASTGoto = ASTStatementBase & {
        type: 'goto';
        label: string;
    };

    type ASTBreak = ASTStatementBase & {
        type: 'break';
        label: string;
    };

    type ASTExit = ASTStatementBase & {
        type: 'exit';
        label: string;
    };

    type ASTContinue = ASTStatementBase & {
        type: 'continue';
        label: string;
    };

    type ASTVarUndef = ASTStatementBase & {
        type: 'varUndef';
        varName: string;
    };

    type ASTAnnotation = ASTStatementBase & {
        type: 'annotation';
        name: string;
        value: string | number;
    };

    type ASTStatement = ASTScopedBlock | ASTIf | ASTWhile | ASTLoop | ASTDeclAssign | ASTDeclMulti
        | ASTDecl | ASTFuncCall | ASTDeclAlias | ASTAssignCalc | ASTNestedCalc
        | ASTLabelDecl | ASTGoto | ASTBreak | ASTExit | ASTContinue | ASTVarUndef | ASTAnnotation;

    type AST = {
        includes: string[];
        state: ASTState[];
        stateData: ASTState[];
        stateBss: ASTState[];
        functions: ASTFunc[];
        postIncludes: string[];
        defines: Record<string, string>;
    };
}
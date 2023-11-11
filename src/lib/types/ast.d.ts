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
    };

    type ASTFunc = {
      type: ASTFuncType;
      resultType?: number;
      name: string;
      body: ASTScopedBlock;
      asm: ASM[];
      args: ASTFuncArg[];
    };

    type ASTMacroMap = Record<string, ASTFunc>;

    type ASTState = {
        arraySize: number[];
        extern: boolean;
        type: string;
        varName: string;
        varType: DataType;
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

    type ASTDeclAssign = ASTStatementBase & {
        type: 'varDeclAssign';
        varName: string;
        calc: ASTCalc;
    };

    type ASTDeclMulti = ASTStatementBase & {
        type: 'varDeclMulti';
        varNames: string[];
        reg: string;
        varType: DataType;
    };

    type ASTDecl = ASTStatementBase & {
        type: 'varDecl';
        varName: string;
        reg: string;
        varType: DataType;
    };

    type ASTDeclAlias = ASTStatementBase & {
        type: 'varDeclAlias';
        varName: string;
        aliasName: string;
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

    type ASTComment = ASTStatementBase & {
        type: 'comment';
        comment: string;
    };

    type ASTLabelDecl = ASTStatementBase & {
        type: 'labelDecl';
        name: string;
    };

    type ASTGoto = ASTStatementBase & {
        type: 'goto';
        label: string;
    };

    type ASTStatement = ASTScopedBlock | ASTIf | ASTWhile | ASTDeclAssign | ASTDeclMulti | ASTDecl | ASTFuncCall | ASTComment | ASTDeclAlias | ASTAssignCalc | ASTLabelDecl | ASTGoto;

    type AST = {
        includes: string[];
        state: ASTState[];
        functions: ASTFunc[];
        postIncludes: string[];
    };
}
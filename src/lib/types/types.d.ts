/**
 * NOTE: this project does NOT use TypeScript.
 * Types defined here are solely used by JSDoc.
 */

declare global {
    type DataType = 'u8' | 's8' | 'u16' | 's16' | 'u32' | 's32' | 'vec16' | 'vec32';
    type CastType = DataType | 'uint' | 'sint' | 'ufract' | 'sfract';
    type Swizzle = string;
    type AnnotationNames = 'Barrier';

    type Annotation = {
        name: AnnotationNames;
        value: string | number;
    };

    type VarRegDef = {
        reg: string;
        type: DataType;
        castType?: CastType;
        originalType?: DataType;
        isConst: boolean;
        modifyCount: number;
    };

    type Scope = {
        varMap: Record<string, VarRegDef>;
        regVarMap: Record<string, string>;
        varAliasMap: Record<string, string>;
        annotations: Annotation[];
        labelStart?: string;
        labelEnd?: string;
    };

    type ScopeStack = Scope[];

    type MemVarDef = {
        name: string;
        type: string;
        arraySize: number;
    };

    type FuncArg = {
        type: DataType;
        reg?: string;
        name: string;
    };

    type FuncDef = {
        name: string;
        args: FuncArg[];
    };

    type RSPLConfig = {
        optimize: boolean;
        reorder: boolean;
        rspqWrapper: boolean;
    };
}
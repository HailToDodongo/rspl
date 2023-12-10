/**
 * NOTE: this project does NOT use TypeScript.
 * Types defined here are solely used by JSDoc.
 */
import "./ast";

declare global
{
    type ASMType = number;

    type ASMDebug = {
        lineASM: number;
        lineRSPL: number;
        reorderLineMin?: ASMDebug;
        reorderLineMax?: ASMDebug;
    };

    type ASM = {
        type: ASMType;
        op: string;
        args: string[];

        label?: string;
        comment?: string;

        depsSource: string[];
        depsTarget: string[];
        debug: ASMDebug;
    };

    type ASMFunc = ASTFunc | {
        asm: ASM[];
        argSize: number;
    };

    type ASMOutputDebug = {
        lineMap: Record<number, number[]>;
        lineDepMap: Record<number, number[]>;
    };

    type ASMOutput = {
      asm: string;
      debug: ASMOutputDebug;
    };
}
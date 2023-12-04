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
    };

    type ASM = {
        type: ASMType;
        op: string;
        args: string[];

        label?: string;
        comment?: string;

        debug: ASMDebug;
    };

    type ASMFunc = ASTFunc | {
        asm: ASM[];
        argSize: number;
    };

    type ASMOutputDebug = {
        lineMap: Record<number, number[]>;
    };

    type ASMOutput = {
      asm: string;
      debug: ASMOutputDebug;
    };
}
/**
 * NOTE: this project does NOT use TypeScript.
 * Types defined here are solely used by JSDoc.
 */
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

    type ASMOutput = {
      asm: string;
      debug: {
        lineMap: Record<number, number[]>;
      }
    };
}
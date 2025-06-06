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
        lineASMOpt: number;
        reorderCount: number;
        reorderLineMin: number;
        reorderLineMax: number;
        cycle: number;
        stall: number;
        paired: boolean;
    };

    type ASM = {
        type: ASMType;
        op: string;
        args: string[];

        label?: string;
        comment?: string;

        depsSourceIdx: number[];
        depsTargetIdx: number[];
        barrierMask: number;

        depsArgMask: BigInt;
        depsSourceMask: BigInt;
        depsTargetMask: BigInt;

        depsBlockSourceMask: BigInt;
        depsBlockTargetMask: BigInt;

        depsStallSourceIdx: number[];
        depsStallTargetIdx: number[];

        depsStallSourceMask0: number;
        depsStallSourceMask1: number;
        depsStallTargetMask0: number;
        depsStallTargetMask1: number;
        opFlags: number;
        labelEnd: string; // sets the end for a block

        stallLatency: number;

        debug: ASMDebug;
        annotations: Annotation[];
    };

    type ASMFunc = ASTFunc | {
        asm: ASM[];
        argSize: number;
        cyclesBefore: number;
        cyclesAfter: number;
    };

    type ASMOutputDebug = {
        lineMap: Record<number, number[]>;
        lineDepMap: Record<number, number[]>;
        lineOptMap: Record<number, number>;
        lineCycleMap: Record<number, number>;
        lineStallMap: Record<number, number>;
    };

    type ASMOutput = {
      asm: string;
      debug: ASMOutputDebug;
      sizeIMEM: number;
      sizeDMEM: number;
    };
}
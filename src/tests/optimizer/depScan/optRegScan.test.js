import {asm} from "../../../lib/intsructions/asmWriter.js";
import {
  asmGetReorderRange,
  asmInitDeps,
  getSourceRegs,
  getSourceRegsFiltered, getTargetRegs
} from "../../../lib/optimizer/asmScanDeps.js";

describe('Optimizer - Register Scanner', () =>
{
  const CASES = {
    "Logic": {
      asm: asm("or", ["$t0", "$a1", "$a0"]),
      src: ["$a1", "$a0"],
      tgt: ["$t0"],
    },
    "Arith": {
      asm: asm("addiu", ["$t0", "$t1", 4]),
      src: ["$t1"],
      tgt: ["$t0"],
    },
    "Vec Store": {
      asm: asm("sdv", ["$v08", 0, 16, "$s6"]),
      src: ["$v08", "$s6"],
      tgt: [],
    },
    "Vec packed Store": {
      asm: asm("sfv", ["$v08", 0, "$s6"]),
      src: ["$v08", "$s6"],
      tgt: [],
    },
  };

  for(const [name, {asm, src, tgt}] of Object.entries(CASES)) {
    test(`Target - ${name}`, () => {
      const target = getTargetRegs(asm);
      expect(target).toEqual(tgt);
    });

    test(`Source - ${name}`, () => {
      const source = getSourceRegsFiltered(asm);
      expect(source).toEqual(src);
    });
  }
});
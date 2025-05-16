import {asm} from "../../../lib/intsructions/asmWriter.js";
import {
  asmGetReorderRange, asmInitDep,
  asmInitDeps,
  getSourceRegs,
  getSourceRegsFiltered, getTargetRegs
} from "../../../lib/optimizer/asmScanDeps.js";

const laneIds = ['_0', '_1', '_2', '_3', '_4', '_5', '_6', '_7'];
function allLanes(reg)
{
  return laneIds.map((id) => reg + id);
}

describe('Optimizer - Register Scanner', () =>
{
  const CASES = {
    "Logic": {
      asm: asm("or", ["$t0", "$a1", "$a0"]),
      src: ["$a1", "$a0"],
      tgt: ["$t0"],
      srcStall: ["$a1", "$a0"],
      tgtStall: ["$t0"],
    },
    "Arith": {
      asm: asm("addiu", ["$t0", "$t1", 4]),
      src: ["$t1"],
      tgt: ["$t0"],
      srcStall: ["$t1"],
      tgtStall: ["$t0"],
    },
    "Vec Store": {
      asm: asm("sdv", ["$v08", 0, 16, "$s6"]),
      src: [...allLanes("$v08"), "$s6"],
      tgt: [],
      srcStall: ["$v08", "$s6"],
      tgtStall: [],
    },
    "Vec packed Store": {
      asm: asm("sfv", ["$v08", 0, "$s6"]),
      src: [...allLanes("$v08"), "$s6"],
      tgt: [],
      srcStall: ["$v08", "$s6"],
      tgtStall: [],
    },
    "Lanes - Vec move": {
      asm: asm("vmov", ["$v07.e3", "$v05.e2"]),
      src: ["$v05_2"],
      tgt: ["$v07_3", "$acc"],
      srcStall: ["$v05"],
      tgtStall: ["$v07"],
    },
  };

  for(const [name, {asm, src, tgt, srcStall, tgtStall}] of Object.entries(CASES)) {
    asmInitDep(asm);

    test(`Source (Logic) - ${name}`, () => {
      expect(asm.depsSource).toEqual(src);
    });

    test(`Target (Logic) - ${name}`, () => {
      expect(asm.depsTarget).toEqual(tgt);
    });

    test(`Source (Stalls) - ${name}`, () => {
      expect(asm.depsStallSource).toEqual(srcStall);
    });

    test(`Target (Stalls) - ${name}`, () => {
      expect(asm.depsStallTarget).toEqual(tgtStall);
    });
  }
});
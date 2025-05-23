import {asm} from "../../../lib/intsructions/asmWriter.js";
import {asmInitDep, REG_STALL_INDEX_MAP} from "../../../lib/optimizer/asmScanDeps.js";

const laneIds = ['_0', '_1', '_2', '_3', '_4', '_5', '_6', '_7'];
function allLanes(reg) {
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
    "Lanes - STV - base": {
      asm: asm("stv", ["$v16", 0, 0, "$t0"]),
      src: ["$v16_0", "$v17_1", "$v18_2", "$v19_3", "$v20_4", "$v21_5", "$v22_6", "$v23_7", "$t0"],
      tgt: [],
      srcStall: ["$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23", "$t0"],
      tgtStall: [],
    },
    "Lanes - STV - offset 2": {
      asm: asm("stv", ["$v08", 2, 0x10, "$t0"]),
      src: ["$v08_7", "$v09_0", "$v10_1", "$v11_2", "$v12_3", "$v13_4", "$v14_5", "$v15_6", "$t0"],
      tgt: [],
      srcStall: ["$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15", "$t0"],
      tgtStall: [],
    },
    "Lanes - STV - offset 8": {
      asm: asm("stv", ["$v08", 8, 0x20, "$t0"]),
      src: ["$v08_4", "$v09_5", "$v10_6", "$v11_7", "$v12_0", "$v13_1", "$v14_2", "$v15_3", "$t0"],
      tgt: [],
      srcStall: ["$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15", "$t0"],
      tgtStall: [],
    },
    "Lanes - LTV - base": {
      asm: asm("ltv", ["$v16", 0, 0, "$t0"]),
      src: ["$t0"],
      tgt: ["$v16_0", "$v17_1", "$v18_2", "$v19_3", "$v20_4", "$v21_5", "$v22_6", "$v23_7"],
      srcStall: ["$t0"],
      tgtStall: ["$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23"],
    },
    "Lanes - LTV - offset 2": {
      asm: asm("ltv", ["$v08", 2, 0x10, "$t0"]),
      src: ["$t0"],
      tgt: ["$v08_7", "$v09_0", "$v10_1", "$v11_2", "$v12_3", "$v13_4", "$v14_5", "$v15_6"],
      srcStall: ["$t0"],
      tgtStall: ["$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15"],
    },
    "Lanes - LTV - offset 8": {
      asm: asm("ltv", ["$v00", 8, 0x20, "$t0"]),
      src: ["$t0"],
      tgt: ["$v00_4", "$v01_5", "$v02_6", "$v03_7", "$v04_0", "$v05_1", "$v06_2", "$v07_3"],
      srcStall: ["$t0"],
      tgtStall: ["$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07"],
    },
    "ctc2 - VCC": {
      asm: asm("ctc2", ["$at", "$vcc"]),
      src: ["$at"],
      tgt: ["$vcc"],
      srcStall: ["$at"],
      tgtStall: [],
    }
  };

  for(const [name, {asm, src, tgt, srcStall, tgtStall}] of Object.entries(CASES)) {
    asmInitDep(asm);
    test(`Source (Logic) - ${name}`,  () => expect(asm.depsSource).toEqual(src));
    test(`Target (Logic) - ${name}`,  () => expect(asm.depsTarget).toEqual(tgt));
    test(`Source (Stalls) - ${name}`, () => expect(asm.depsStallSourceIdx).toEqual(srcStall.map(r => REG_STALL_INDEX_MAP[r])));
    test(`Target (Stalls) - ${name}`, () => expect(asm.depsStallTargetIdx).toEqual(tgtStall.map(r => REG_STALL_INDEX_MAP[r])));
  }
});
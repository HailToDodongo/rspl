import {asm} from "../lib/intsructions/asmWriter.js";
import {asmInitDep, asmGetReorderIndices, getTargetRegs} from "../lib/optimizer/asmScanDeps.js";

function build() {
  const list = [
    asm("vmadn", ["$v06", "$v14", "$v30.e2"]),   // 0: dead full write to $v06
    asm("ldv",   ["$v06", 0, 0, "$s0"]),          // 1: sV lanes 0-3
    asm("ldv",   ["$v06", 8, 0, "$s1"]),          // 2: sV lanes 4-7
    asm("vmulf", ["$v04", "$v06", "$v06"]),       // 3: reads sV
  ];
  for(const a of list) { a.annotations = a.annotations || []; asmInitDep(a); }
  return list;
}

test("Reorder partial load", () => {
  const list = build();

  // sanity: does the partial ldv even list $v06 as a *source* (preserved lanes)?
  console.log("ldv $v06,8 source regs :", JSON.stringify(list[2].depsSourceIdx));
  console.log("ldv $v06,8 target regs :", JSON.stringify(list[2].depsTargetIdx.slice(0,4)), "...");

  const range = asmGetReorderIndices(list, 0);   // where can the dead vmadn (idx 0) go?
  const min = Math.min(...range), max = Math.max(...range);
  const canLandBetweenLdvs = range.includes(1);   // index 1 == AFTER ldv[0], BEFORE ldv[8]
  expect(canLandBetweenLdvs).toBe(false);   // this SHOULD hold; it currently FAILS -> bug
});

test("Partial load target lanes - aligned offset", () => {
  expect(getTargetRegs(asm("ldv", ["$v06", 0, 0, "$s0"])))
    .toEqual(["$v06.e0", "$v06.e1", "$v06.e2", "$v06.e3"]);
  expect(getTargetRegs(asm("ldv", ["$v06", 8, 0, "$s0"])))
    .toEqual(["$v06.e4", "$v06.e5", "$v06.e6", "$v06.e7"]);
  expect(getTargetRegs(asm("llv", ["$v06", 4, 0, "$s0"])))
    .toEqual(["$v06.e2", "$v06.e3"]);
  expect(getTargetRegs(asm("lsv", ["$v06", 2, 0, "$s0"])))
    .toEqual(["$v06.e1"]);
  expect(getTargetRegs(asm("lbv", ["$v06", 4, 0, "$s0"])))
    .toEqual(["$v06.e2"]);
});

test("Partial load target lanes - odd offset", () => {
  // an odd offset straddles a lane boundary, touching one extra lane
  expect(getTargetRegs(asm("lbv", ["$v06", 5, 0, "$s0"])))
    .toEqual(["$v06.e2"]);
  expect(getTargetRegs(asm("lsv", ["$v06", 3, 0, "$s0"])))
    .toEqual(["$v06.e1", "$v06.e2"]);
  expect(getTargetRegs(asm("llv", ["$v06", 1, 0, "$s0"])))
    .toEqual(["$v06.e0", "$v06.e1", "$v06.e2"]);
  // loads stop at the end of the register, lanes must be clamped
  expect(getTargetRegs(asm("ldv", ["$v06", 13, 0, "$s0"])))
    .toEqual(["$v06.e6", "$v06.e7"]);
  expect(getTargetRegs(asm("lsv", ["$v06", 15, 0, "$s0"])))
    .toEqual(["$v06.e7"]);
});

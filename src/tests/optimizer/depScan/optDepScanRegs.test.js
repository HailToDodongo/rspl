import {asm} from "../../../lib/intsructions/asmWriter.js";
import {asmGetReorderRange, asmInitDeps} from "../../../lib/optimizer/asmScanDeps.js";

function asmLinesToDeps(lines)
{
  asmInitDeps({asm: lines});
  return lines.map((line, i) => asmGetReorderRange(lines, i));
}

describe('Optimizer - Dependency Scanner', () =>
{
  test('No Deps', () => {
    const lines = [
      /* 00 */ asm("or", ["$t0", "$zero", "$zero"]),
      /* 01 */ asm("or", ["$t1", "$zero", "$zero"]),
      /* 02 */ asm("or", ["$t2", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 2],
      [0, 2],
      [0, 2],
    ]);
  });

  test('Basic Write Dep', () => {
    const lines = [
      /* 00 */ asm("or", ["$t0", "$zero", "$zero"]), // writes to 2's input
      /* 01 */ asm("or", ["$t1", "$zero", "$zero"]),
      /* 02 */ asm("or", ["$t2", "$t0",   "$zero"]), // needs 0's output
      /* 03 */ asm("or", ["$t3", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 1],
      [0, 3],
      [1, 3],
      [0, 3],
    ]);
  });

  test('Nested Write Dep', () => {
    const lines = [
      /* 00 */ asm("or", ["$t0", "$zero", "$zero"]), // writes to 2's input
      /* 01 */ asm("or", ["$t1", "$zero", "$zero"]),
      /* 02 */ asm("or", ["$t2", "$t0",   "$zero"]), // needs 0's output
      /* 03 */ asm("or", ["$t3", "$t2",   "$zero"]), // needs 3's output
      /* 04 */ asm("or", ["$t4", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 1],
      [0, 4],
      [1, 2],
      [3, 4],
      [0, 4],
    ]);
  });

  test('MTC2 (partial write)', () => {
    const lines = [
      /* 00 */ asm("vxor", ["$v25", "$v00", "$v00.e0"]),
      /* 01 */ asm("addiu", ["$at", "$zero", 3]),
      /* 02 */ asm("mtc2", ["$at", "$v25.e6"]),
    ];

    expect(asmLinesToDeps(lines)).toEqual([
      [0, 2],
      [0, 1],
      [2, 2],
    ]);
  });

  test('MTC2 (partial write, no return regs)', () => {
    const lines = [
      /* 00 */ asm("vxor", ["$v25", "$v00", "$v00.e0"]),
      /* 01 */ asm("vxor", ["$v26", "$v00", "$v00"]),
      /* 02 */ asm("mtc2", ["$at", "$v25.e6"]),
    ];

    expect(asmLinesToDeps(lines)).toEqual([
      [0, 2],
      [1, 2], // @TODO: allow 0-1
      [1, 2],
    ]);
  });

    test('MTC2 (partial write, return regs)', () => {
    const lines = [
      /* 00 */ asm("vxor", ["$v25", "$v00", "$v00.e0"]),
      /* 01 */ asm("vxor", ["$v26", "$v00", "$v00"]),
      /* 02 */ asm("mtc2", ["$at", "$v25.e6"]),
    ];

    expect(asmLinesToDeps(lines, ["$v25_6"])).toEqual([
      [0, 1],
      [1, 2], // @TODO: allow 0-1
      [1, 2],
    ]);
  });

  test('Ignore Write when no read (simple)', () => {
    const lines = [
      /* 00 */ asm("or", ["$t0", "$zero", "$zero"]),
      /* 01 */ asm("or", ["$t0", "$zero", "$zero"]),
      /* 02 */ asm("or", ["$t0", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 2],
      [0, 2],
      [2, 2],
    ]);
  });

  test('Ignore Write when no read (deps)', () => {
    const lines = [
      /* 00 */ asm("or", ["$t0", "$zero", "$zero"]),
      /* 01 */ asm("or", ["$t0", "$zero", "$zero"]),
      /* 02 */ asm("or", ["$t1", "$t0",   "$zero"]), // needs 1's output
      /* 03 */ asm("or", ["$t0", "$zero", "$zero"]),
      /* 04 */ asm("or", ["$t0", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 0],
      [1, 1],
      [2, 2],
      [3, 4],
      [4, 4],
    ]);
  });

  test('Hidden Regs (simple)', () => {
    const lines = [
      /* 00 */ asm("veq",  ["$v11", "$v00",  "$v00" ]), // sets VCC
      /* 01 */ asm("or",   ["$t0",  "$zero", "$zero"]),
      /* 02 */ asm("or",   ["$t0",  "$zero", "$zero"]),
      /* 03 */ asm("vmrg", ["$v01", "$v02",  "$v03" ]), // reads VCC
      /* 04 */ asm("or",   ["$t0",  "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 2],
      [0, 4],
      [0, 4],
      [1, 4],
      [3, 4],
    ]);
  });

  test('Regs Single-Lane', () => {
    const lines = [
      /* 00 */ asm("vmov",  ["$v11.e1", "$v05.e1"]),
      /* 01 */ asm("vmov",  ["$v06.e1", "$v11.e2"]), // same ref, different lane as 0's target
      /* 02 */ asm("vmov",  ["$v07.e1", "$v11.e1"]), // actual dep
      /* 03 */ asm("vmov",  ["$v08.e1", "$v05.e1"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 1],
      [0, 3],
      [1, 3],
      [3, 3],
    ]);
  });

  test('Offset Syntax', () => {
    const lines = [
      /* 00 */ asm("or", ["$t0", "$zero", "$zero"]),
      /* 01 */ asm("or", ["$t1", "$zero", "$zero"]),
      /* 02 */ asm("lw", ["$t2", "0($t0)"]),
      /* 02 */ asm("or", ["$t3", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 1],
      [0, 3],
      [1, 3],
      [0, 3],
    ]);
  });

  test('Vector Example (mul + add)', () => {
    const lines = [
      /* 00 */ asm("vmudl", ["$v27", "$v18", "$v26.v"]),
      /* 01 */ asm("vmadm", ["$v27", "$v17", "$v26.v"]),
      /* 02 */ asm("vmadn", ["$v18", "$v18", "$v25.v"]),
      /* 03 */ asm("vmadh", ["$v17", "$v17", "$v25.v"]),
      /* 04 */ asm("vaddc", ["$v18", "$v18", "$v24.v"]),
      /* 05 */ asm("vadd",  ["$v17", "$v17", "$v23.v"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0,0],
      [1,1],
      [2,2],
      [3,3],
      [4,4],
      [5,5],
    ]);
  });
});
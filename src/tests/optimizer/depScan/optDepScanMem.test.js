import {asm, asmLabel} from "../../../lib/intsructions/asmWriter.js";
import {asmGetReorderIndices, asmInitDeps} from "../../../lib/optimizer/asmScanDeps.js";
import state from "../../../lib/state.js";

function asmLinesToDeps(lines)
{
  asmInitDeps({asm: lines});
  return lines.map((line, i) => asmGetReorderIndices(lines, i));
}

describe('Optimizer - Dependency Scanner - Memory', () =>
{
  test('Read vs. Read', () => {
    const lines = [
      /* 00 */ asm("lw", ["$t0", "0($s1)"]),
      /* 01 */ asm("or", ["$t1", "$zero", "$zero"]),
      /* 02 */ asm("lw", ["$t2", "0($s1)"]),
      /* 03 */ asm("or", ["$t3", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 1, 2, 3],
      [0, 1, 2, 3],
      [0, 1, 2, 3],
      [0, 1, 2, 3],
    ]);
  });

  test('Read vs. Write', () => {
    const lines = [
      /* 00 */ asm("lw", ["$t0", "0($s1)"]),
      /* 01 */ asm("or", ["$t1", "$zero", "$zero"]),
      /* 02 */ asm("sw", ["$t2", "0($s1)"]),
      /* 03 */ asm("or", ["$t3", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 1, 2, 3],
      [0, 1, 2, 3],
      [0, 1, 2, 3],
      [0, 1, 2, 3],
    ]);
  });

  test('Read vs. Write (Barrier)', () => {
    const lines = [
      /* 00 */ asm("lw", ["$t0", "0($s1)"]),
      /* 01 */ asm("or", ["$t1", "$zero", "$zero"]),
      /* 02 */ asm("sw", ["$t2", "0($s1)"]),
      /* 03 */ asm("or", ["$t3", "$zero", "$zero"]),
    ];
    lines[0].annotations = [{name: "Barrier", value: "some barrier"}];
    lines[2].annotations = [{name: "Barrier", value: "some barrier"}];

    state.reset();
    state.enterFunction("test", "command", 0);
    state.pushScope();

    expect(asmLinesToDeps(lines)).toEqual([
      [0, 1],
      [0, 1, 2, 3],
      [1, 2, 3],
      [0, 1, 2, 3],
    ]);
  });

  test('Write vs. Write', () => {
    const lines = [
      /* 00 */ asm("sw", ["$t0", "0($s2)"]),
      /* 01 */ asm("or", ["$t1", "$zero", "$zero"]),
      /* 02 */ asm("sw", ["$t2", "0($s1)"]),
      /* 03 */ asm("or", ["$t3", "$zero", "$zero"]),
    ];
    expect(asmLinesToDeps(lines)).toEqual([
      [0, 1, 2, 3],
      [0, 1, 2, 3],
      [0, 1, 2, 3],
      [0, 1, 2, 3],
    ]);
  });

});
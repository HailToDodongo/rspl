import {asm} from "../../../lib/intsructions/asmWriter.js";
import {asmGetReorderIndices, asmInitDeps} from "../../../lib/optimizer/asmScanDeps.js";
import state from "../../../lib/state.js";

function withBarrier(inst, tag, mode = "") {
  inst.annotations = [{name: "Barrier", value: tag, mode}];
  return inst;
}

function asmLinesToDeps(lines)
{
  state.reset();
  asmInitDeps({asm: lines});
  return lines.map((line, i) => asmGetReorderIndices(lines, i).sort((a, b) => a - b));
}

describe('Optimizer - Dep Scanner barrier-modes', () =>
{
  test('strict', () => {
    const lines = [
      /* 00 */ withBarrier(asm("sw", ["$t0", "0($s1)"]), "dma"),
      /* 01 */ withBarrier(asm("sw", ["$t1", "4($s1)"]), "dma"),
      /* 02 */ asm("or", ["$t3", "$zero", "$zero"]),
      /* 03 */ withBarrier(asm("mtc0", ["$t2", "COP0_DMA_WRITE"]), "dma"),
    ];
    const deps = asmLinesToDeps(lines);
    expect(deps[0]).not.toContain(2); // cannot pass the second store
    expect(deps[1]).not.toContain(0);
    expect(deps[3]).not.toContain(1); // the consumer stays behind both stores
    expect(deps[3]).toContain(2);     // (the untagged op in between is fine)
  });

  test('before/after', () => {
    const lines = [
      /* 00 */ withBarrier(asm("sw", ["$t0", "0($s1)"]), "dma", "before"),
      /* 01 */ withBarrier(asm("sw", ["$t1", "4($s1)"]), "dma", "before"),
      /* 02 */ asm("or", ["$t3", "$zero", "$zero"]),
      /* 03 */ withBarrier(asm("mtc0", ["$t2", "COP0_DMA_WRITE"]), "dma", "after"),
      /* 04 */ asm("or", ["$t4", "$zero", "$zero"]),
    ];
    const deps = asmLinesToDeps(lines);
    expect(deps[0]).toContain(2);       // may pass its sibling store ...
    expect(deps[1]).toContain(0);
    expect(deps[0]).not.toContain(4);   // ... but never the consumer
    expect(deps[1]).not.toContain(4);
    expect(deps[3]).not.toContain(0);   // the consumer cannot move above a store
    expect(deps[3]).not.toContain(1);
    expect(deps[3]).toContain(4);       // but is free below
  });

  test('independent tags', () => {
    const lines = [
      /* 00 */ withBarrier(asm("sw", ["$t0", "0($s1)"]), "a", "before"),
      /* 01 */ withBarrier(asm("mtc0", ["$t2", "COP0_DMA_WRITE"]), "b", "after"),
    ];
    const deps = asmLinesToDeps(lines);
    expect(deps[0]).toContain(1);
  });
});

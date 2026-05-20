import {dedupeLabels} from "../lib/optimizer/pattern/dedupeLabels.js";
import {ASM_TYPE} from "../lib/intsructions/asmWriter.js";

function L(name) {
  return { type: ASM_TYPE.LABEL, op: "", label: name, args: [], opFlags: 0, annotations: [] };
}
function O(op, args = []) {
  return { type: ASM_TYPE.OP, op, args, label: "", opFlags: 0, annotations: [] };
}
function B(op, args, labelEnd) {
  return { type: ASM_TYPE.OP, op, args, label: "", labelEnd, opFlags: 0, annotations: [] };
}

describe('Optimizer - dedupeLabels', () => {

  test('Consecutive labels are deduplicated to the last one', () => {
    let func = { asm: [
      B("j", ["LABEL_A"], "LABEL_A"),
      O("nop"),
      L("LABEL_A"),
      L("LABEL_B"),
      O("addiu", ["$t0", "$zero", "1"]),
    ]};
    dedupeLabels(func);
    // LABEL_B kept, LABEL_A deleted, branch patched to LABEL_B
    const labels = func.asm.filter(a => a.label).map(a => a.label);
    expect(labels).toEqual(["LABEL_B"]);
    expect(func.asm[0].args[0]).toBe("LABEL_B");
    // Note: JS only patches args, not labelEnd
  });

  test('__-prefixed labels are NOT deduplicated', () => {
    let func = { asm: [
      B("j", ["SKIP"], "SKIP"),
      O("nop"),
      L("__A"),
      L("__A"),
      L("__A"),
      O("addiu", ["$t0", "$zero", "1"]),
    ]};
    dedupeLabels(func);
    // All three __ labels preserved
    const labels = func.asm.filter(a => a.label).map(a => a.label);
    expect(labels).toEqual(["__A", "__A", "__A"]);
  });

  test('__ label breaks deduplication chain', () => {
    // __B in the middle prevents SKIP dedup
    let func = { asm: [
      B("j", ["SKIP"], "SKIP"),
      O("nop"),
      L("SKIP"),
      L("__B"),
      L("SKIP"),
      O("addiu", ["$t0", "$zero", "1"]),
    ]};
    dedupeLabels(func);
    // __B preserved, SKIP labels handled per-segment
    const labels = func.asm.filter(a => a.label).map(a => a.label);
    expect(labels).toContain("__B");
    // Branch should still reference SKIP
    expect(func.asm[0].args[0]).toBe("SKIP");
  });

});

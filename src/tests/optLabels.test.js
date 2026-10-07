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

    let func = { name: "test", asm: [
      B("j", ["LABEL_test_0001"], "LABEL_test_0001"),
      O("nop"),
      L("LABEL_test_0001"),
      L("LABEL_test_0002"),
      O("addiu", ["$t0", "$zero", "1"]),
    ]};
    dedupeLabels(func);
    expect(func.asm.length).toBe(4);
    expect(func.asm[0].args[0]).toBe("LABEL_test_0002");
    expect(func.asm[0].labelEnd).toBe("LABEL_test_0002");
    expect(func.asm[2].label).toBe("LABEL_test_0002");
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

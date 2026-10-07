import {transpileSource} from "../../../lib/transpiler";
import {dedupeLabels} from "../../../lib/optimizer/pattern/dedupeLabels.js";
import * as asmOptimizer from "../../../lib/optimizer/asmOptimizer.js";
import {ASM_TYPE} from "../../../lib/intsructions/asmWriter.js";

function L(name) {
  return { type: ASM_TYPE.LABEL, op: "", label: name, args: [], opFlags: 0, annotations: [] };
}
function O(op, args = []) {
  return { type: ASM_TYPE.OP, op, args, label: "", opFlags: 0, annotations: [] };
}
function B(op, args, labelEnd) {
  return { type: ASM_TYPE.OP, op, args, label: "", labelEnd, opFlags: 0, annotations: [] };
}

const CONF = {rspqWrapper: false, optimize: true};

describe('Optimizer E2E - Labels', () =>
{
  test('De-dupe Labels', async () => {
    const {asm, warn} = await transpileSource(`function test(u32 dummy) 
    {
      LABEL_A:
      LABEL_B:
      LABEL_C:
      goto LABEL_A;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  LABEL_A:
  LABEL_B:
  LABEL_C:
  j LABEL_A
  nop`);
  });

  test('De-dupe Labels - keep single', async () => {
    const {asm, warn} = await transpileSource(`function test(u32 dummy) 
    {
      LABEL_A:
      dummy += 1;
      LABEL_B:
      dummy += 2;
      LABEL_C:
      goto LABEL_A;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  LABEL_A:
  addiu $a0, $a0, 1
  LABEL_B:
  addiu $a0, $a0, 2
  LABEL_C:
  j LABEL_A
  nop`);
  });
});

describe('Optimizer - dedupeLabels', () =>
{
  test('a generated label folds into the user label, either order', () => {
    for(const generatedFirst of [true, false]) {
      const func = { name: "test", asm: [
        B("j", ["LABEL_test_0001"], "LABEL_test_0001"),
        O("nop"),
        L(generatedFirst ? "LABEL_test_0001" : "USER"),
        L(generatedFirst ? "USER" : "LABEL_test_0001"),
        O("addiu", ["$t0", "$zero", "1"]),
      ]};
      dedupeLabels(func);
      expect(func.asm.length).toBe(4);
      expect(func.asm[0].args[0]).toBe("USER");
      expect(func.asm[0].labelEnd).toBe("USER");
      expect(func.asm[2].label).toBe("USER");
    }
  });

  test('two user labels are both kept', () => {
    const func = { name: "test", asm: [
      B("j", ["USER_A"], "USER_A"),
      O("nop"),
      L("USER_A"),
      L("USER_B"),
      O("addiu", ["$t0", "$zero", "1"]),
    ]};
    dedupeLabels(func);
    expect(func.asm.length).toBe(5);
    expect(func.asm[0].args[0]).toBe("USER_A");
    expect(func.asm[2].label).toBe("USER_A");
    expect(func.asm[3].label).toBe("USER_B");
  });
});

describe('Optimizer - isGeneratedLabel', () =>
{
  test('isGeneratedLabel', () => {
    const func = { name: "test", asm: [] };
    
    expect(asmOptimizer.isGeneratedLabel(func, "LABEL_test_0001")).toBe(true);
    expect(asmOptimizer.isGeneratedLabel(func, "LABEL_test_00FF")).toBe(true);
    expect(asmOptimizer.isGeneratedLabel(func, "LABEL_A")).toBe(false);
    expect(asmOptimizer.isGeneratedLabel(func, "LABEL_other_0001")).toBe(false);
    expect(asmOptimizer.isGeneratedLabel(func, "LABEL_test_001")).toBe(false);
    expect(asmOptimizer.isGeneratedLabel(func, "LABEL_test_00G1")).toBe(false);
  });
});

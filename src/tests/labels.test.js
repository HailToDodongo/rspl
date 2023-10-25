import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Labels', () =>
{
  test('Basic Labels', () => {
    const {asm, warn} = transpileSource(`
      function test_label()
      {
        label_a:
        label_b: label_c:
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_label:
  label_a:
  label_b:
  label_c:
  jr ra`);
  });

  test('Labels with instr.', () => {
    const {asm, warn} = transpileSource(`
      function test_label()
      {
        u32<$t0> a;
        label_a:
          a += 1; 
          goto label_b;
        label_b:
          a += 2; 
          goto label_a;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_label:
  label_a:
  addiu t0, t0, 1
  b label_b
  nop
  label_b:
  addiu t0, t0, 2
  b label_a
  nop
  jr ra`);
  });
});
import {transpileSource} from "../../../lib/transpiler";

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
  LABEL_C:
  j LABEL_C
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
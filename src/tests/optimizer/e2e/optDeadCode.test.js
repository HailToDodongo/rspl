import {transpileSource} from "../../../lib/transpiler";

const CONF = {rspqWrapper: false, optimize: true};

describe('Optimizer E2E - Dead Code', () =>
{
  test('Jump at end - safe', async () => {
    const {asm, warn} = await transpileSource(`function test() 
    {
      goto TEST;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  j TEST
  nop`);
  });

  test('Jump at end - unsafe with code', async () => {
    const {asm, warn} = await transpileSource(`function test() 
    {
      goto TEST;
      u32 x = 2;
      x = 3;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  j TEST
  nop
  addiu $t0, $zero, 3
  jr $ra
  addiu $t0, $zero, 2`);
  });

  test('Jump at end - unsafe', async () => {
    const {asm, warn} = await transpileSource(`
    function test2();
    function test() 
    {
      test2();
      u32 x = 2;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  jal test2
  nop
  jr $ra
  addiu $t0, $zero, 2`);
  });

  test('Jump in branch - unsafe', async () => {
    const {asm, warn} = await transpileSource(`
    function test2();
    function test() 
    {
      u32 x = 1;
      if(x) {
        test2();
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $t0, $zero, 1
  beq $t0, $zero, LABEL_0001
  nop
  jal test2
  nop
  LABEL_0001:
  jr $ra
  nop`);
  });
});
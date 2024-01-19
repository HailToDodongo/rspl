import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Syntax - Numbers', () =>
{
  test('Scalar - Assignment', async () => {
    const {asm, warn} = await transpileSource(`function test() 
{
  u32<$t0> a;
  a = 1234;
  a = 0x1234;
  a = 0b1010;
}`, CONF);

    expect(asm).toBe(`test:
  addiu $t0, $zero, 1234
  addiu $t0, $zero, 4660
  addiu $t0, $zero, 10
  jr $ra
  nop`);
  });

  test('Scalar - Calc', async () => {
    const {asm, warn} = await transpileSource(`function test() 
{
  u32<$t0> a;
  a = a + 1234;
  a = a + 0x1234;
  a = a + 0b1010;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $t0, $t0, 1234
  addiu $t0, $t0, 4660
  addiu $t0, $t0, 10
  jr $ra
  nop`);
  });
});
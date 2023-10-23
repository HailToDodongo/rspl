import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Syntax - Numbers', () =>
{
  test('Scalar - Assignment', () => {
    const asm = transpileSource(`function test() 
{
  u32<$t0> a;
  a = 1234;
  a = 0x1234;
  a = 0b1010;
}`, CONF).trim();

    expect(asm).toBe(`test:
  li t0, 0x04D2
  li t0, 0x1234
  li t0, 0x000A
  jr ra`);
  });

  test('Scalar - Calc', () => {
    const asm = transpileSource(`function test() 
{
  u32<$t0> a;
  a = a + 1234;
  a = a + 0x1234;
  a = a + 0b1010;
}`, CONF).trim();

    expect(asm).toBe(`test:
  addiu t0, t0, 1234
  addiu t0, t0, 4660
  addiu t0, t0, 10
  jr ra`);
  });

  test('Scalar - Invalid (float)', () => {
    const src = `function test() {
      u32<$t0> a = 1.25;
    }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/invalid syntax at line 2/);
  });
});
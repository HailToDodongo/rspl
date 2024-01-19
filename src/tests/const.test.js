import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Const', () =>
{
  test('Const Declaration', async () => {
    const {asm, warn} = await transpileSource(`function test() 
    {
      const u32<$t0> a = 1234;
      const u32<$t1> b = a + a;
      const vec16<$v01> c = 0;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $t0, $zero, 1234
  addu $t1, $t0, $t0
  vxor $v01, $v00, $v00.e0
  jr $ra
  nop`);
  });

  test('Const invalid (scalar)', async () =>
  {
    const src = `function test() {
      const u32<$t0> a = 1234;
      a += 1;
    }`;
   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 3: Cannot assign to constant variable!/);
  });

  test('Const invalid (vector)', async () =>
  {
    const src = `function test() {
      const vec16<$v01> a = 0;
      a += a;
    }`;
   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 3: Cannot assign to constant variable!/);
  });
});
import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Scope', () =>
{
  test('Var Declaration', async () => {
    const {asm, warn} = await transpileSource(`function test_scope() 
{
  u32<$t0> a;
  {
     u32<$t1> b;
     b += 2;
  } // 'b' is no longer defined now
  a += 2;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_scope:
  addiu $t1, $t1, 2
  addiu $t0, $t0, 2
  jr $ra
  nop`);
  });

  test('Var Un-Declaration', async () => {
    const src = `function test_scope() 
    {
      u32<$t0> a;
      a += 2;
      undef a;
      a = 2;
    }`;
   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 6: result Variable a not known!/);
  });

  test('Var Decl. invalid', async () =>
  {
    const src = `function test_scope() 
    {
      u32<$t0> a;
      {
         u32<$t1> b;
         b += 2;
      }
      b += 2;
    }`;
   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 8: result Variable b not known!/);
  });
});
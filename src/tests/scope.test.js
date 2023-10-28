import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Scope', () =>
{
  test('Var Declaration', () => {
    const {asm, warn} = transpileSource(`function test_scope() 
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
  ## 'b' is no longer defined now
  addiu $t0, $t0, 2
  jr $ra
  nop`);
  });

  test('Var Decl. invalid', () =>
  {
    const src = `function test_scope() 
    {
      u32<$t0> a;
      {
         u32<$t1> b;
         b += 2;
      } // 'b' is no longer defined now
      b += 2;
    }`;
   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 8: result Variable b not known!/);
  });
});
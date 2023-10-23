import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Syntax - Swizzle', () =>
{
  test('Invalid on Scalar (calc)', () => {
    const src = `function test() {
      u32<$t0> a;
      a += a.x;
    }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 3: Swizzling not allowed for scalar operations!/);
  });

  test('Invalid on Scalar (assign)', () => {
    const src = `function test() {
      u32<$t0> a;
      a = a.x;
    }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 3: Swizzling not allowed for scalar operations!/);
  });
});
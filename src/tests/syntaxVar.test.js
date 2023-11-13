import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Syntax - Vars', () =>
{
  test('Declare - Invalid (type scalar)', () => {
    const src = `function test() {
      u32<$v03> a;
    }`;

    expect(() => transpileSource(src, CONF))
      .toThrowError(/line 2: Cannot use vector register for scalar variable!/);
  });

  test('Declare - Invalid (vector scalar)', () => {
    const src = `function test() {
      vec16<$t0> a;
    }`;

    expect(() => transpileSource(src, CONF))
      .toThrowError(/line 2: Cannot use scalar register for vector variable!/);
  });

  test('Declare - Invalid (swizzle)', () => {
    const src = `function test() {
      vec16<$v03> a.x;
    }`;

    expect(() => transpileSource(src, CONF))
      .toThrowError(/Syntax error at line 2/);
  });

  test('Declare - Invalid (cast)', () => {
    const src = `function test() {
      vec16<$v03> a:sint;
    }`;

    expect(() => transpileSource(src, CONF))
      .toThrowError(/line 2: Variable name cannot contain a cast \(':'\)!/);
  });
});
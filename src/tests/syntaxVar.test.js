import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Syntax - Vars', () =>
{
  test('Declare - Invalid (type scalar)', async () => {
    const src = `function test() {
      u32<$v03> a;
    }`;

    await expect(async () => transpileSource(src, CONF))
      .rejects.toThrowError(/line 2: Cannot use vector register for scalar variable!/);
  });

  test('Declare - Invalid (vector scalar)', async () => {
    const src = `function test() {
      vec16<$t0> a;
    }`;

    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/line 2: Cannot use scalar register for vector variable!/);
  });

  test('Declare - Invalid (swizzle)', async () => {
    const src = `function test() {
      vec16<$v03> a.x;
    }`;

    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Syntax error at line 2/);
  });

  test('Declare - Invalid (cast)', async () => {
    const src = `function test() {
      vec16<$v03> a:sint;
    }`;

    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/line 2: Variable name cannot contain a cast \(':'\)!/);
  });
});
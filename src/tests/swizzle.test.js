import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Syntax - Swizzle', () =>
{
  test('Assign single (vec32 <- vec32)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> a, b;
      a.x = b.X;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v03.e4
  vmov $v02.e0, $v04.e4
  jr $ra
  nop`);
  });

  test('Assign single (vec32 <- vec32, cast)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> a, b;
      SINT:
      a.x = b:sint.X;        // all <- sint
      a:sint.x = b:sint.X;   // sint <- sint
      a:ufract.x = b:sint.X; // ufract <- sint
      
      UFRACT:
      a.x = b:ufract.X;        // all <- ufract
      a:sint.x = b:ufract.X;   // sint <- ufract
      a:ufract.x = b:ufract.X; // ufract <- ufract
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  SINT:
  vmov $v01.e0, $v03.e4
  vmov $v02.e0, $v00.e4
  vmov $v01.e0, $v03.e4
  vmov $v02.e0, $v03.e4
  UFRACT:
  vmov $v01.e0, $v00.e4
  vmov $v02.e0, $v04.e4
  vmov $v01.e0, $v04.e4
  vmov $v02.e0, $v04.e4
  jr $ra
  nop`);
  });

  test('Assign single (vec16 <- vec16)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec16<$v01> a, b;
      a.x = b.X;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v02.e4
  jr $ra
  nop`);
  });

  test('Assign single (vec32 <- vec16)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> a;
      vec16<$v03> b;
      a.x = b.X;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v03.e4
  vmov $v02.e0, $v00.e4
  jr $ra
  nop`);
  });

  test('Assign single (vec16 <- vec32)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a.x = b.X;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v02.e4
  jr $ra
  nop`);
  });

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
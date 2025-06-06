import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Syntax - Swizzle', () =>
{
  test('Assign single (vec32 <- vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
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

  test('Assign single (vec32 <- vec32, cast)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
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

  test('Assign single (vec16 <- vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a, b;
      a.x = b.X;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v02.e4
  jr $ra
  nop`);
  });

  test('Assign single (vec32 <- vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
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

  test('Assign single (vec16 <- vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
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

  test('Invalid on Scalar (calc)', async () => {
    const src = `function test() {
      u32<$t0> a;
      a += a.x;
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 3: Swizzling not allowed for scalar operations!/);
  });

  test('Invalid on Scalar (assign)', async () => {
    const src = `function test() {
      u32<$t0> a;
      a = a.x;
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 3: Swizzling not allowed for scalar operations!/);
  });

  test('Alias (integer index)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a;
      a.x = a.0;
      a.1 = a.z;
      
      a += a.xxzzXXZZ;
      a += a.00224466;
      
      a += a.wwwwWWWW;
      a += a.33337777;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v01.e0
  vmov $v01.e1, $v01.e2
  vaddc $v01, $v01, $v01.q0
  vaddc $v01, $v01, $v01.q0
  vaddc $v01, $v01, $v01.h3
  vaddc $v01, $v01, $v01.h3
  jr $ra
  nop`);
  });
});
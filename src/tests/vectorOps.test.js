import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Vector - Ops', () =>
{
  test('Add (vec32 vs vec32)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> res, a;
      res += a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vaddc $v02, $v02, $v04.e0
  vadd $v01, $v01, $v03.e0
  jr $ra
  nop`);
  });

  test('Add (vec16 vs vec16)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec16<$v01> res, a;
      res += a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vaddc $v01, $v01, $v02.e0
  jr $ra
  nop`);
  });

  test('Sub (vec32 vs vec32)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> res, a;
      res -= a.y;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vsubc $v02, $v02, $v04.e1
  vsub $v01, $v01, $v03.e1
  jr $ra
  nop`);
  });

  test('Sub (vec16 vs vec16)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec16<$v01> res, a;
      res -= a;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vsubc $v01, $v01, $v02.v
  jr $ra
  nop`);
  });

    test('Mul (vec32 vs vec32)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> res, a;
      res *= a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v27, $v02, $v04.e0
  vmadm $v27, $v01, $v04.e0
  vmadn $v02, $v02, $v03.e0
  vmadh $v01, $v01, $v03.e0
  jr $ra
  nop`);
  });

  test('Invert (vec32)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> res, a;
      a.x = invertHalf(a).x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vrcph $v03.e0, $v03.e0
  vrcpl $v04.e0, $v04.e0
  vrcph $v03.e0, $v00.e0
  jr $ra
  nop`);
  });

  test('Div (vec32 vs vec32)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> res, a;
      res /= a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vrcph $v28.e0, $v03.e0
  vrcpl $v29.e0, $v04.e0
  vrcph $v28.e0, $v00.e0
  vmudn $v29, $v29, $v30.e6
  vmadh $v28, $v28, $v30.e6
  vmudl $v27, $v02, $v29.e0
  vmadm $v27, $v01, $v29.e0
  vmadn $v02, $v02, $v28.e0
  vmadh $v01, $v01, $v28.e0
  jr $ra
  nop`);
  });

});
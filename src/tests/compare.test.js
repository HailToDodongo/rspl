import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Comparison', () =>
{
  test('Vector (vec16 vs vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a < b;
      res = a >= b;
      res = a == b;
      res = a != b;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vlt $v01, $v02, $v03
  vge $v01, $v02, $v03
  veq $v01, $v02, $v03
  vne $v01, $v02, $v03
  jr $ra
  nop`);
  });

  test('Vector (vec16 vs const)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a < 0;
      res = a >= 2;
      res = a == 32;
      res = a != 256;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vlt $v01, $v02, $v00.e0
  vge $v01, $v02, $v30.e6
  veq $v01, $v02, $v30.e2
  vne $v01, $v02, $v31.e7
  jr $ra
  nop`);
  });

  test('Vector-Select (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = select(a, b);
      res = select(a, 32);
      // res = select(64, b); // INVALID
      // res = select(2, 4);  // INVALID
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmrg $v01, $v02, $v03
  vmrg $v01, $v02, $v30.e2
  jr $ra
  nop`);
  });

  test('Vector-Select (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a, b;
      A:
      res = select(a, b);
      B:
      res = select(a, b.y);
      C:
      res = select(a, 32);
      // res = select(64, b); // INVALID
      // res = select(2, 4);  // INVALID
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  A:
  vmrg $v01, $v03, $v05
  vmrg $v02, $v04, $v06
  B:
  vmrg $v01, $v03, $v05.e1
  vmrg $v02, $v04, $v06.e1
  C:
  vmrg $v01, $v03, $v30.e2
  vmrg $v02, $v04, $v00.e2
  jr $ra
  nop`);
  });

  test('Vector-Select (vec32 cast)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a, b;
      
      res:sint = select(a, b:sfract);
      res:sfract = select(a, 32);
      
      // res = select(64, b); // INVALID
      // res = select(2, 4);  // INVALID
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmrg $v01, $v03, $v06
  vmrg $v02, $v04, $v00.e2
  jr $ra
  nop`);
  });

  test('Vector-Ternary (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      vec16<$v10> x, y;
      
      A:
      res = x != y ? a : b;
      B:
      res = x != 4 ? a : 32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  A:
  vne $v29, $v10, $v11
  vmrg $v01, $v02, $v03
  B:
  vne $v29, $v10, $v30.e5
  vmrg $v01, $v02, $v30.e2
  jr $ra
  nop`);
  });

  test('Vector-Ternary (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a, b;
      vec16<$v10> x, y;
      
      A:
      res = x != y ? a : b;
      B:
      res = x != 4 ? a : 32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  A:
  vne $v29, $v10, $v11
  vmrg $v01, $v03, $v05
  vmrg $v02, $v04, $v06
  B:
  vne $v29, $v10, $v30.e5
  vmrg $v01, $v03, $v30.e2
  vmrg $v02, $v04, $v00.e2
  jr $ra
  nop`);
  });

  test('Vector-Ternary (swizzle)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      A:
      res = a == b ? a : b.y;
      B:
      res = a >= b.z ? a : b.y;
      C:
      res = a == b.z ? a : b;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  A:
  veq $v29, $v02, $v03
  vmrg $v01, $v02, $v03.e1
  B:
  vge $v29, $v02, $v03.e2
  vmrg $v01, $v02, $v03.e1
  C:
  veq $v29, $v02, $v03.e2
  vmrg $v01, $v02, $v03
  jr $ra
  nop`);
  });
});
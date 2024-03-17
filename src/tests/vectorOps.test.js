import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Vector - Ops', () =>
{
  test('Assign (vec32 vs vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a;
      res = a;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vor $v01, $v00, $v03
  vor $v02, $v00, $v04
  jr $ra
  nop`);
  });

  test('Assign (vec16 vs vec32:cast)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res;
      vec32<$v03> a;
      res = a:uint;
      res = a:sint;
      res:ufract = a:ufract;
      res:sfract = a:sfract;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vor $v01, $v00, $v03
  vor $v01, $v00, $v03
  vor $v01, $v00, $v04
  vor $v01, $v00, $v04
  jr $ra
  nop`);
  });

  test('Assign (vec16 vs vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a;
      res = a;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vor $v01, $v00, $v02
  jr $ra
  nop`);
  });

  test('Assign (vec16 broadcast)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a;
      res = a.yyyyYYYY;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vor $v01, $v00, $v02.h1
  jr $ra
  nop`);
  });

    test('Assign (vec32 broadcast)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a;
      res = a.yyyyYYYY;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vor $v01, $v00, $v03.h1
  vor $v02, $v00, $v04.h1
  jr $ra
  nop`);
  });

  test('Assign (swizzle, 2^x)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a.x = 2;
      b.x = 8;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v30.e6
  vmov $v02.e0, $v30.e4
  vmov $v03.e0, $v00.e0
  jr $ra
  nop`);
  });

  test('Assign (swizzle, float)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a.x = 10.25;
      b.x = 42.125;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $at, $zero, 10
  mtc2 $at, $v01.e0
  addiu $at, $zero, 42
  mtc2 $at, $v02.e0
  addiu $at, $zero, 8192
  mtc2 $at, $v03.e0
  jr $ra
  nop`);
  });

  test('Assign (swizzle, int-variable)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32 s;
      vec16<$v01> a;
      vec32<$v02> b;
      a.y = s;
      b.z = s;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mtc2 $t0, $v01.e1
  mtc2 $t0, $v03.e2
  srl $at, $t0, 16
  mtc2 $at, $v02.e2
  jr $ra
  nop`);
  });

  test('Assign (no-swizzle, int-variable)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32 s;
      vec16<$v01> a;
      vec32<$v02> b;
      a = s;
      b = s;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mtc2 $t0, $v01.e0
  vor $v01, $v00, $v01.e0
  mtc2 $t0, $v03.e0
  srl $at, $t0, 16
  mtc2 $at, $v02.e0
  vor $v02, $v00, $v02.e0
  vor $v03, $v00, $v03.e0
  jr $ra
  nop`);
  });

  test('Assign (swizzle, 0)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a.x = 0;
      b.x = 0;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v00.e0
  vmov $v02.e0, $v00.e0
  vmov $v03.e0, $v00.e0
  jr $ra
  nop`);
  });

  test('Assign (cast, swizzle, 0)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a:sint.x = 0;
      b:sfract.x = 0;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmov $v01.e0, $v00.e0
  vmov $v03.e0, $v00.e0
  jr $ra
  nop`);
  });

  test('Assign (0)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a = 0;
      vec32<$v02> b = 0;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v01, $v00, $v00.e0
  vxor $v02, $v00, $v00.e0
  vxor $v03, $v00, $v00
  jr $ra
  nop`);
  });

  test('Add (vec32 vs vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
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

  test('Add (vec16 vs vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a;
      res += a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vaddc $v01, $v01, $v02.e0
  jr $ra
  nop`);
  });

  test('Add (vec16 cast)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a;
      res:uint += a.x;
      res:sint += a.x;
      res:sfract += a.x;
      res:ufract += a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vaddc $v01, $v01, $v02.e0
  vadd $v01, $v01, $v02.e0
  vadd $v01, $v01, $v02.e0
  vaddc $v01, $v01, $v02.e0
  jr $ra
  nop`);
  });

  test('Sub (vec32 vs vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
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

  test('Sub (vec16 vs vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a;
      res -= a;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vsubc $v01, $v01, $v02.v
  jr $ra
  nop`);
  });

    test('Mul (vec32 vs vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a;
      res *= a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v29, $v02, $v04.e0
  vmadm $v29, $v01, $v04.e0
  vmadn $v02, $v02, $v03.e0
  vmadh $v01, $v01, $v03.e0
  jr $ra
  nop`);
  });

  test('Mul (vec16 vs vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a;
      res *= a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudn $v01, $v01, $v02.e0
  jr $ra
  nop`);
  });

  test('Mul (vec16 cast)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a;
      res:uint *= a.x;
      res:sint *= a.x;
      res:ufract *= a.x;
      res:sfract *= a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudn $v01, $v01, $v02.e0
  vmudn $v01, $v01, $v02.e0
  vmulu $v01, $v01, $v02.e0
  vmulf $v01, $v01, $v02.e0
  jr $ra
  nop`);
  });

  test('AND (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v02> res16, a16;
      vec32<$v04> a32;
      
      res16 = a16 & a16;
      res16 = a32 & a16;
      res16 = a16 & a32;
      res16 = a32 & a32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vand $v02, $v03, $v03.v
  vand $v02, $v04, $v03.v
  vand $v02, $v03, $v04.v
  vand $v02, $v04, $v04.v
  jr $ra
  nop`);
  });

  test('AND (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> res32, a32;
      vec16<$v06> a16;
      
      res32 = a16 & a16; A:
      res32 = a32 & a16; B:
      res32 = a16 & a32; C:
      res32 = a32 & a32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vand $v02, $v06, $v06.v
  vand $v03, $v00, $v00.v
  A:
  vand $v02, $v04, $v06.v
  vand $v03, $v05, $v00.v
  B:
  vand $v02, $v06, $v04.v
  vand $v03, $v00, $v05.v
  C:
  vand $v02, $v04, $v04.v
  vand $v03, $v05, $v05.v
  jr $ra
  nop`);
  });

  test('OR (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v02> res16, a16;
      vec32<$v04> a32;
      
      res16 = a16 | a16;
      res16 = a32 | a16;
      res16 = a16 | a32;
      res16 = a32 | a32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vor $v02, $v03, $v03.v
  vor $v02, $v04, $v03.v
  vor $v02, $v03, $v04.v
  vor $v02, $v04, $v04.v
  jr $ra
  nop`);
  });

  test('OR (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> res32, a32;
      vec16<$v06> a16;
      
      res32 = a16 | a16; AA:
      res32 = a32 | a16; BB:
      res32 = a16 | a32; CC:
      res32 = a32 | a32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vor $v02, $v06, $v06.v
  vor $v03, $v00, $v00.v
  AA:
  vor $v02, $v04, $v06.v
  vor $v03, $v05, $v00.v
  BB:
  vor $v02, $v06, $v04.v
  vor $v03, $v00, $v05.v
  CC:
  vor $v02, $v04, $v04.v
  vor $v03, $v05, $v05.v
  jr $ra
  nop`);
  });

  test('XOR (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v02> res16, a16;
      vec32<$v04> a32;
      
      res16 = a16 ^ a16;
      res16 = a32 ^ a16;
      res16 = a16 ^ a32;
      res16 = a32 ^ a32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v02, $v03, $v03.v
  vxor $v02, $v04, $v03.v
  vxor $v02, $v03, $v04.v
  vxor $v02, $v04, $v04.v
  jr $ra
  nop`);
  });

  test('XOR (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> res32, a32;
      vec16<$v06> a16;
      
      res32 = a16 ^ a16; A:
      res32 = a32 ^ a16; B:
      res32 = a16 ^ a32; C:
      res32 = a32 ^ a32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v02, $v06, $v06.v
  vxor $v03, $v00, $v00.v
  A:
  vxor $v02, $v04, $v06.v
  vxor $v03, $v05, $v00.v
  B:
  vxor $v02, $v06, $v04.v
  vxor $v03, $v00, $v05.v
  C:
  vxor $v02, $v04, $v04.v
  vxor $v03, $v05, $v05.v
  jr $ra
  nop`);
  });

  test('NOT (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v02> res16, a16;
      vec32<$v04> a32;
      
      res16 = ~a16;
      res16 = ~a32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vnor $v02, $v03, $v00.v
  vnor $v02, $v04, $v00.v
  jr $ra
  nop`);
  });

  test('NOT (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> res32, a32;
      vec16<$v06> a16;
      
      res32 = ~a16;
      res32 = ~a32;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vnor $v02, $v06, $v00.v
  vnor $v03, $v00, $v00.v
  vnor $v02, $v04, $v00.v
  vnor $v03, $v05, $v00.v
  jr $ra
  nop`);
  });

  test('Invert-Half (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a;
      a.x = invert_half(a).x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vrcph $v03.e0, $v03.e0
  vrcpl $v04.e0, $v04.e0
  vrcph $v03.e0, $v00.e0
  jr $ra
  nop`);
  });

  test('Invert-Half - all (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a;
      a = invert_half(a);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vrcph $v03.e0, $v03.e0
  vrcpl $v04.e0, $v04.e0
  vrcph $v03.e0, $v00.e0
  vrcph $v03.e1, $v03.e1
  vrcpl $v04.e1, $v04.e1
  vrcph $v03.e1, $v00.e1
  vrcph $v03.e2, $v03.e2
  vrcpl $v04.e2, $v04.e2
  vrcph $v03.e2, $v00.e2
  vrcph $v03.e3, $v03.e3
  vrcpl $v04.e3, $v04.e3
  vrcph $v03.e3, $v00.e3
  vrcph $v03.e4, $v03.e4
  vrcpl $v04.e4, $v04.e4
  vrcph $v03.e4, $v00.e4
  vrcph $v03.e5, $v03.e5
  vrcpl $v04.e5, $v04.e5
  vrcph $v03.e5, $v00.e5
  vrcph $v03.e6, $v03.e6
  vrcpl $v04.e6, $v04.e6
  vrcph $v03.e6, $v00.e6
  vrcph $v03.e7, $v03.e7
  vrcpl $v04.e7, $v04.e7
  vrcph $v03.e7, $v00.e7
  jr $ra
  nop`);
  });

  test('Invert-SQRT-Half (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a;
      a.x = invert_half_sqrt(a).x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vrsqh $v03.e0, $v03.e0
  vrsql $v04.e0, $v04.e0
  vrsqh $v03.e0, $v00.e0
  jr $ra
  nop`);
  });

  test('Invert (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res, a;
      a = invert(a);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vrcph $v03.e0, $v03.e0
  vrcpl $v04.e0, $v04.e0
  vrcph $v03.e0, $v00.e0
  vrcph $v03.e1, $v03.e1
  vrcpl $v04.e1, $v04.e1
  vrcph $v03.e1, $v00.e1
  vrcph $v03.e2, $v03.e2
  vrcpl $v04.e2, $v04.e2
  vrcph $v03.e2, $v00.e2
  vrcph $v03.e3, $v03.e3
  vrcpl $v04.e3, $v04.e3
  vrcph $v03.e3, $v00.e3
  vrcph $v03.e4, $v03.e4
  vrcpl $v04.e4, $v04.e4
  vrcph $v03.e4, $v00.e4
  vrcph $v03.e5, $v03.e5
  vrcpl $v04.e5, $v04.e5
  vrcph $v03.e5, $v00.e5
  vrcph $v03.e6, $v03.e6
  vrcpl $v04.e6, $v04.e6
  vrcph $v03.e6, $v00.e6
  vrcph $v03.e7, $v03.e7
  vrcpl $v04.e7, $v04.e7
  vrcph $v03.e7, $v00.e7
  vmudn $v04, $v04, $v30.e6
  vmadh $v03, $v03, $v30.e6
  jr $ra
  nop`);
  });

  /*test('Div (vec32 vs vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
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
  vmudl $v29, $v02, $v29.e0
  vmadm $v29, $v01, $v29.e0
  vmadn $v02, $v02, $v28.e0
  vmadh $v01, $v01, $v28.e0
  jr $ra
  nop`);
  });*/

});
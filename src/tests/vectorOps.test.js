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
      res:sfract += a.x; // unexpected?
      res:ufract += a.x; // unexpected?
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vaddc $v01, $v01, $v02.e0
  vadd $v01, $v01, $v02.e0
  vadd $v01, $v01, $v00.e0
  vaddc $v01, $v01, $v00.e0
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

  test('Mul (vec32 vs vec32:ufract)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec32<$v01> res, a;
      res *= a:ufract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v02, $v02, $v04.e0
  vmadm $v01, $v01, $v04.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop`);
  });

  test('Mul (vec16 vs vec32:ufract)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec32<$v01> res, a;
      vec16<$v05> b;
      res = b * a:ufract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudm $v01, $v05, $v04.e0
  vmadn $v02, $v00, $v00
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

  test('Mul (vec16 vs vec16 -> vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res;
      vec16<$v03> a, b;
      res = a * b.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudh $v02, $v03, $v04.e0
  vsar $v01, COP2_ACC_HI
  vsar $v02, COP2_ACC_MD
  jr $ra
  nop`);
  });

  test('Mul (vec16 vs vec32)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec32<$v01> res, a;
      vec16<$v05> b;
      res = b * a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudm $v02, $v05, $v04.e0
  vmadh $v01, $v05, $v03.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop`);
  });

  test('Mul (vec16 vs vec32 -> vec16)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec16<$v01> res, a;
      vec32<$v03> b;
      res = a * b.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudm $v01, $v02, $v04.e0
  vmadh $v01, $v02, $v03.e0
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
  vmudh $v01, $v01, $v02.e0
  vmulu $v01, $v01, $v02.e0
  vmulf $v01, $v01, $v02.e0
  jr $ra
  nop`);
  });

  test('Mul (vec16:sint vs vec16:sint)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:sint * b:sint.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudh $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Mul (vec16:ufract vs vec16:ufract)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:ufract * b:ufract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmulu $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Mul (vec16:sfract vs vec16:sfract)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:sfract * b:sfract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmulf $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Mul (vec16:sint vs vec16:ufract)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:sint * b:ufract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudm $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Mul (vec16:ufract vs vec16:sint)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:ufract * b:sint.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudn $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Add-Mul (vec32 vs vec32)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec32<$v01> res, a;
      res = res +* a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadl $v29, $v02, $v04.e0
  vmadm $v29, $v01, $v04.e0
  vmadn $v02, $v02, $v03.e0
  vmadh $v01, $v01, $v03.e0
  jr $ra
  nop`);
  });

  test('Add-Mul (vec32 vs vec32:ufract)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec32<$v01> res, a;
      res = res +* a:ufract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadl $v02, $v02, $v04.e0
  vmadm $v01, $v01, $v04.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16 vs vec32:ufract)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec32<$v01> res, a;
      vec16<$v05> b;
      res = b +* a:ufract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadm $v01, $v05, $v04.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop`);
  });

  test('Add-Mul (vec32 vs vec32, cast sfract)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec32<$v01> res, a;
      res:sfract = res +* a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadl $v29, $v02, $v04.e0
  vmadm $v29, $v01, $v04.e0
  vmadn $v02, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16 vs vec16 -> vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> res;
      vec16<$v03> a, b;
      res = a +* b.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadh $v02, $v03, $v04.e0
  vsar $v01, COP2_ACC_HI
  vsar $v02, COP2_ACC_MD
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16 vs vec32)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec32<$v01> res, a;
      vec16<$v05> b;
      res = b +* a.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadm $v02, $v05, $v04.e0
  vmadh $v01, $v05, $v03.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16 vs vec32 -> vec16)', async () => {
    const { asm, warn } = await transpileSource(`function test() {
      vec16<$v01> res, a;
      vec32<$v03> b;
      res = a +* b.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadm $v01, $v02, $v04.e0
  vmadh $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16:sint vs vec16:sint)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:sint +* b:sint.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadh $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16:ufract vs vec16:ufract)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:ufract +* b:ufract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmacu $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16:sfract vs vec16:sfract)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:sfract +* b:sfract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmacf $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16:sint vs vec16:ufract)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:sint +* b:ufract.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadm $v01, $v02, $v03.e0
  jr $ra
  nop`);
  });

  test('Add-Mul (vec16:ufract vs vec16:sint)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a:ufract +* b:sint.x;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadn $v01, $v02, $v03.e0
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

  test('Shift Left (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v02> a, b;
      b = a << 1;
      b = a << 4;
      b = a << 15;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudn $v03, $v02, $v30.e6
  vmudn $v03, $v02, $v30.e3
  vmudn $v03, $v02, $v31.e0
  jr $ra
  nop`);
  });

  test('Shift Right Arithmetic (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v02> a, b;
      b = a >> 1;
      b = a >> 4;
      b = a >> 15;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudm $v03, $v02, $v31.e0
  vmudm $v03, $v02, $v31.e3
  vmudm $v03, $v02, $v30.e6
  jr $ra
  nop`);
  });

  test('Shift Right Logical (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v02> a, b;
      b = a >>> 1;
      b = a >>> 4;
      b = a >>> 15;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v03, $v02, $v31.e0
  vmudl $v03, $v02, $v31.e3
  vmudl $v03, $v02, $v30.e6
  jr $ra
  nop`);
  });

  test('Shift Left (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> a, b;
      b = a << 1;
      b = a << 4;
      b = a << 15;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v04, $v03, $v30.e6
  vmadn $v04, $v02, $v30.e6
  vmudn $v05, $v03, $v30.e6
  vmudl $v04, $v03, $v30.e3
  vmadn $v04, $v02, $v30.e3
  vmudn $v05, $v03, $v30.e3
  vmudl $v04, $v03, $v31.e0
  vmadn $v04, $v02, $v31.e0
  vmudn $v05, $v03, $v31.e0
  jr $ra
  nop`);
  });

  test('Shift Left (vec32 self-assign)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> a, b;
      a = a << 1;
      a = a << 4;
      a = a << 15;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v29, $v03, $v30.e6
  vmadn $v02, $v02, $v30.e6
  vmudn $v03, $v03, $v30.e6
  vmudl $v29, $v03, $v30.e3
  vmadn $v02, $v02, $v30.e3
  vmudn $v03, $v03, $v30.e3
  vmudl $v29, $v03, $v31.e0
  vmadn $v02, $v02, $v31.e0
  vmudn $v03, $v03, $v31.e0
  jr $ra
  nop`);
  });

  test('Shift Left (vec16 = vec32 << X)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> b;
      vec16<$v04> a;
      a = b << 1;
      a = b << 4;
      a = b << 15;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v04, $v03, $v30.e6
  vmadn $v04, $v02, $v30.e6
  vmudl $v04, $v03, $v30.e3
  vmadn $v04, $v02, $v30.e3
  vmudl $v04, $v03, $v31.e0
  vmadn $v04, $v02, $v31.e0
  jr $ra
  nop`);
  });

  test('Shift right Arithmetic (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> a, b;
      b = a >> 1;
      b = a >> 4;
      b = a >> 15;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v05, $v03, $v31.e0
  vmadm $v04, $v02, $v31.e0
  vmadn $v05, $v00, $v00
  vmudl $v05, $v03, $v31.e3
  vmadm $v04, $v02, $v31.e3
  vmadn $v05, $v00, $v00
  vmudl $v05, $v03, $v30.e6
  vmadm $v04, $v02, $v30.e6
  vmadn $v05, $v00, $v00
  jr $ra
  nop`);
  });

  test('Shift right Logical (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> a, b;
      b = a >>> 1;
      b = a >>> 4;
      b = a >>> 15;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudl $v05, $v03, $v31.e0
  vmadn $v04, $v02, $v31.e0
  vmadn $v05, $v00, $v00
  vmudl $v05, $v03, $v31.e3
  vmadn $v04, $v02, $v31.e3
  vmadn $v05, $v00, $v00
  vmudl $v05, $v03, $v30.e6
  vmadn $v04, $v02, $v30.e6
  vmadn $v05, $v00, $v00
  jr $ra
  nop`);
  });

  test('Multiply-accumulate +* - vec32', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec32<$v01> a;
        vec32<$v03> b;
        vec32<$v05> res;
        res = a +* b;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadl $v05, $v02, $v04.v
  vmadm $v05, $v01, $v04.v
  vmadn $v06, $v02, $v03.v
  vmadh $v05, $v01, $v03.v
  jr $ra
  nop`);
  });

  test('Multiply vec16 * vec32 -> vec32', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec16<$v01> a;
        vec32<$v03> b;
        vec32<$v05> res;
        res = a * b;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudn $v06, $v04, $v01.v
  vmadh $v05, $v03, $v01.v
  jr $ra
  nop`);
  });

  test('Half-move vec32 xyzw=XYZW (upper to lower)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec32<$v01> res, a;
        res.xyzw = a.XYZW;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)
  sdv $v03, 8, 0, $at
  sdv $v04, 8, 8, $at
  ldv $v01, 0, 0, $at
  ldv $v02, 0, 8, $at
  jr $ra
  nop`);
  });

  test('Half-move vec32 XYZW=xyzw (lower to upper)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec32<$v01> res, a;
        res.XYZW = a.xyzw;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)
  sdv $v03, 0, 0, $at
  sdv $v04, 0, 8, $at
  ldv $v01, 8, 0, $at
  ldv $v02, 8, 8, $at
  jr $ra
  nop`);
  });

  test('Half-move vec16 xyzw=XYZW (upper to lower)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec16<$v01> res, a;
        res.xyzw = a.XYZW;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)
  sdv $v02, 8, 0, $at
  ldv $v01, 0, 0, $at
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
    // an operand without a cast follows the destination's view
    expect(asm).toBe(`test:
  vaddc $v01, $v01, $v02.e0
  vadd $v01, $v01, $v02.e0
  vadd $v01, $v01, $v02.e0
  vaddc $v01, $v01, $v02.e0
  jr $ra
  nop`);
  });

  test('Multiply vec16 * vec32 with a swizzled vec32', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec16<$v01> a;
        vec32<$v03> b;
        vec32<$v05> res;
        res = a * b.wwwwWWWW;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudm $v06, $v01, $v04.h3
  vmadh $v05, $v01, $v03.h3
  vmadn $v06, $v00, $v00
  jr $ra
  nop`);
  });

  test('Multiply rejects a swizzle on the left operand', async () => {
    const src = `function test() {
        vec16<$v01> a;
        vec32<$v03> b;
        vec32<$v05> res;
        res = b.wwwwWWWW * a;
      }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/swizzle on the right side/);
  });

  test('Logic with pow2 constant (vec16)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a;
      a &= 0x400;
      a |= 16;
      a ^= 0x8000;
      a = a & 2;
      a &= 0;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vand $v01, $v01, $v31.e5
  vor $v01, $v01, $v30.e3
  vxor $v01, $v01, $v31.e0
  vand $v01, $v01, $v30.e6
  vand $v01, $v01, $v00.e0
  jr $ra
  nop`);
  });

  test('Logic with pow2 constant (vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> b;
      b &= 0x400;
      b ^= 4;
    }`, CONF);

    expect(warn).toBe("");
    // the constant's fraction bits are zero -> fract half uses the zero lane
    expect(asm).toBe(`test:
  vand $v02, $v02, $v31.e5
  vand $v03, $v03, $v00.e0
  vxor $v02, $v02, $v30.e5
  vxor $v03, $v03, $v00.e0
  jr $ra
  nop`);
  });

  test('Logic with non-pow2 constant throws', async () => {
    const src = `function test() {
      vec16<$v01> a;
      a &= 3;
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/powers of two/);
  });

  test('Add on vec32 fraction view with uncast operand', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> nrT;
      vec16<$v16> normShift;
      nrT:ufract += normShift.x;
      nrT:ufract += 2;
      nrT:sfract += normShift.x;
      nrT:ufract += normShift:ufract.x;
      nrT:sint += normShift.x;
      nrT:ufract -= normShift.x;
      nrT += normShift.x;
    }`, CONF);

    expect(warn).toBe("");
    // the view decides how an uncast operand is read.
    // a full vec32 target keeps reading it as the integer half (with the carry into the int add)
    expect(asm).toBe(`test:
  vaddc $v03, $v03, $v16.e0
  vaddc $v03, $v03, $v30.e6
  vadd $v03, $v03, $v16.e0
  vaddc $v03, $v03, $v16.e0
  vadd $v02, $v02, $v16.e0
  vsubc $v03, $v03, $v16.e0
  vaddc $v03, $v03, $v00.e0
  vadd $v02, $v02, $v16.e0
  jr $ra
  nop`);
  });

  test('Add with an operand cast to the opposite view throws', async () => {
    await expect(() => transpileSource(`function test() {
      vec32<$v02> nrT;
      vec16<$v16> a;
      nrT:ufract += a:sint.x;
    }`, CONF)).rejects.toThrowError(/integer operand on a fraction view/);

    await expect(() => transpileSource(`function test() {
      vec32<$v02> nrT;
      vec16<$v16> a;
      nrT:sint += a:ufract.x;
    }`, CONF)).rejects.toThrowError(/fraction operand on an integer view/);
  });

  test('Mul (vec16 fraction * vec32)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v02> nrT;
      vec32<$v13> screenSize;
      vec16<$v01> nrD;
      vec16<$v05> r16;
      nrT = nrD:ufract * screenSize.wwwwWWWW;
      r16 = nrD:ufract * screenSize.wwwwWWWW;
      nrT = screenSize * nrD:ufract.xxxxXXXX;
    }`, CONF);

    expect(warn).toBe("");
s
    expect(asm).toBe(`test:
  vmudl $v02, $v01, $v14.h3
  vmadn $v03, $v01, $v13.h3
  vmadh $v02, $v00, $v00
  vmudl $v05, $v01, $v14.h3
  vmadn $v05, $v01, $v13.h3
  vmadh $v05, $v00, $v00
  vmudl $v03, $v14, $v01.h0
  vmadm $v02, $v13, $v01.h0
  vmadn $v03, $v00, $v00
  jr $ra
  nop`);
  });

  test('scratch uses the result register, not VTEMP', async () => {
    const {asm, warn} = await transpileSource(`function test()
{
  vec32<$v05> posClip;
  vec16<$v15> guardBandScale;
  vec16<$v02> clipPlaneW = posClip * guardBandScale.xxxxxxxx;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudn $v02, $v06, $v15.e0
  vmadh $v02, $v05, $v15.e0
  jr $ra
  nop`);
  });

  test('scratch keeps VTEMP when the result is also a source', async () => {
    const {asm, warn} = await transpileSource(`function test()
{
  vec32<$v05> big;
  vec16<$v02> out;
  out = big * out.xxxxxxxx;
}`, CONF);

    expect(warn).toBe("");
    // $v02 is read by the vmadh, so it cannot take the scratch write
    expect(asm).toBe(`test:
  vmudn $v29, $v06, $v02.e0
  vmadh $v02, $v05, $v02.e0
  jr $ra
  nop`);
  });

  test('a live VTEMP survives an unrelated multiply', async () => {
    const {asm, warn} = await transpileSource(`function test()
{
  vec16<$v10> a;
  vec16<$v11> b;
  vec16<$v12> c;
  vec32<$v05> big;
  vec16<$v02> out;
  VTEMP = a * b;
  out = big * c.xxxxxxxx;
  a:sint += VTEMP;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmudn $v29, $v10, $v11.v
  vmudn $v02, $v06, $v12.e0
  vmadh $v02, $v05, $v12.e0
  vadd $v10, $v10, $v29.v
  jr $ra
  nop`);
  });
});

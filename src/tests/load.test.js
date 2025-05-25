import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Load', () =>
{
  test('Scalar - 32-Bit', async () => {
    const {asm, warn} = await transpileSource(` state { u32 TEST_CONST; }
      function test_scalar_load()
      {
        u32<$t0> src, dst;

        dst = load(src);
        dst = load(src, 0x10);
        dst = load(src, TEST_CONST);
        
        dst = load(TEST_CONST);
        dst = load(TEST_CONST, 0x10);
        // dst = load(TEST_CONST, TEST_CONST); Invalid
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_scalar_load:
  lw $t1, 0($t0)
  lw $t1, 16($t0)
  lw $t1, %lo(TEST_CONST)($t0)
  lw $t1, %lo(TEST_CONST + 0)
  lw $t1, %lo(TEST_CONST + 16)
  jr $ra
  nop`);
  });

  test('Scalar - Cast', async () => {
    const {asm, warn} = await transpileSource(`
      function test_scalar_load()
      {
        u32<$t0> src, dst;

        dst:u32 = load(src, 0x10);
        dst:u16 = load(src, 0x10);
        dst:u8 = load(src, 0x10);
        
        dst:s32 = load(src, 0x10);
        dst:s16 = load(src, 0x10);
        dst:s8 = load(src, 0x10);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_scalar_load:
  lw $t1, 16($t0)
  lhu $t1, 16($t0)
  lbu $t1, 16($t0)
  lw $t1, 16($t0)
  lh $t1, 16($t0)
  lb $t1, 16($t0)
  jr $ra
  nop`);
  });

  test('Vector - 32-Bit', async () => {
    const {asm, warn} = await transpileSource(` state { u32 TEST_CONST; }
      function test_vector_load() 
      {
        u32<$t0> src;
        vec32<$v01> dst;
        
        WholeVector:
        dst = load(src);
        dst = load(src, 0x10);
        dst.y = load(src);
        dst.z = load(src, 0x10);
        //dst = load(src, TEST_CONST); Invalid
        //dst = load(TEST_CONST); Invalid
        //dst = load(TEST_CONST, 0x10); Invalid
        
        Swizzle:
        dst = load(src).xyzwxyzw;
        dst = load(src, 0x10).xyzwxyzw;
        dst.y = load(src).xyzwxyzw;
        dst.z = load(src, 0x10).xyzwxyzw;
        //dst = load(src, TEST_CONST).xyzwxyzw; Invalid
        //dst = load(TEST_CONST).xyzwxyzw; Invalid
        //dst = load(TEST_CONST, 0x10).xyzwxyzw; Invalid
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_vector_load:
  WholeVector:
  lqv $v01, 0, 0, $t0
  lqv $v02, 0, 16, $t0
  lqv $v01, 0, 16, $t0
  lqv $v02, 0, 32, $t0
  lqv $v01, 2, 0, $t0
  lqv $v02, 2, 16, $t0
  lqv $v01, 4, 16, $t0
  lqv $v02, 4, 32, $t0
  Swizzle:
  ldv $v01, 0, 0, $t0
  ldv $v01, 8, 0, $t0
  ldv $v02, 0, 8, $t0
  ldv $v02, 8, 8, $t0
  ldv $v01, 0, 16, $t0
  ldv $v01, 8, 16, $t0
  ldv $v02, 0, 24, $t0
  ldv $v02, 8, 24, $t0
  ldv $v01, 2, 0, $t0
  ldv $v01, 10, 0, $t0
  ldv $v02, 2, 8, $t0
  ldv $v02, 10, 8, $t0
  ldv $v01, 4, 16, $t0
  ldv $v01, 12, 16, $t0
  ldv $v02, 4, 24, $t0
  ldv $v02, 12, 24, $t0
  jr $ra
  nop`);
  });

  test('Vector - 32-Bit Split', async () => {
    const {asm, warn} = await transpileSource(`function test() 
      {
        u32<$t0> src;
        vec32<$v01> dst;
        
        LeftSide:
        dst.xyzw = load(src, 0x00).xyzw;
        dst.xyzw = load(src, 0x10).xyzw;
        
        RightSide:
        dst.XYZW = load(src, 0x00).XYZW;
        dst.XYZW = load(src, 0x10).XYZW;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  LeftSide:
  ldv $v01, 0, 0, $t0
  ldv $v02, 0, 8, $t0
  ldv $v01, 0, 16, $t0
  ldv $v02, 0, 24, $t0
  RightSide:
  ldv $v01, 8, 8, $t0
  ldv $v02, 8, 16, $t0
  ldv $v01, 8, 24, $t0
  ldv $v02, 8, 32, $t0
  jr $ra
  nop`);
  });

  test('Vector - Cast', async () => {
    const {asm, warn} = await transpileSource(`function test() 
      {
        u32<$t0> addr;
        vec32<$v01> dst;
        
        dst:sint   = load(addr, 0x10);
        dst:ufract = load(addr, 0x10);
        
        dst:sint   = load(addr, 0x10).XY;
        dst:ufract = load(addr, 0x10).XY;
        
        dst:sint.z   = load(addr, 0x10).XY;
        dst:ufract.z = load(addr, 0x10).XY;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  lqv $v01, 0, 16, $t0
  lqv $v02, 0, 16, $t0
  llv $v01, 0, 24, $t0
  llv $v02, 0, 24, $t0
  llv $v01, 4, 24, $t0
  llv $v02, 4, 24, $t0
  jr $ra
  nop`);
  });

 test('Vector - Packed-Load', async () => {
    const {asm, warn} = await transpileSource(`function test() 
      {
        u32<$t0> src;
        vec16<$v01> dst;
        
        Unsigned:
        dst = load_vec_u8(src, 0x00);
        dst.z = load_vec_u8(src, 0x10);
        
        Signed:
        dst.x = load_vec_s8(src, 0x00);
        dst.z = load_vec_s8(src, 0x10);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  Unsigned:
  luv $v01, 0, 0, $t0
  luv $v01, 2, 16, $t0
  Signed:
  lpv $v01, 0, 0, $t0
  lpv $v01, 2, 16, $t0
  jr $ra
  nop`);
  });

  test('Vector - Transposed-Load', async () => {
    const {asm, warn} = await transpileSource(`function test() 
      {
        u32<$t0> ptr;
        vec16<$v08> a;
        vec16<$v16> b;
        
        a = load_transposed(0, ptr, 0x00);
        a = load_transposed(0, ptr);
        a = load_transposed(1, ptr, 0x10);
        b = load_transposed(4, ptr, 0x20);
        b = load_transposed(7, ptr, 0x30);
        END:
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ltv $v08, 0, 0, $t0
  ltv $v08, 0, 0, $t0
  ltv $v08, 2, 16, $t0
  ltv $v16, 8, 32, $t0
  ltv $v16, 14, 48, $t0
  END:
  jr $ra
  nop`);
  });

  test('Invalid Transpose Load - reg', async () => {
    const src = `function test() {
      u32<$t0> ptr;
      vec32<$v04> v;
      v = load_transposed(0, ptr, 0x00);
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/Error in test, line 4: Builtin load_transposed\(\) requires result register to be \$v00, \$v08, \$v16 or \$v24!/);
  });

  test('Invalid Transpose Load - offset', async () => {
    const src = `function test() {
      u32<$t0> ptr;
      vec32<$v16> v;
      v = load_transposed(0, ptr, 0x04);
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/Error in test, line 4: Builtin load_transposed\(\) requires offset to be multiple of 16/);
  });

  test('Invalid vector load (const not % 16)', async () => {
    const src = `function test() {
      u32<$t0> a;
      vec16 err = load(a, 5);
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 3: Invalid full vector-load offset, must be a multiple of 16, 5 given/);
  });

  test('Invalid vector load (vector as addr)', async () => {
    const src = `
    state {
      u32 TEST_CONST;
    }
    function test() {
      vec32<$v01> a; 
      a = load(a, TEST_CONST);
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/Error in test, line 7: Builtin load\(\) requires first argument to be a scalar!/);
  });
});
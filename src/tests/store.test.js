import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Store', () =>
{
  test('Scalar - 32-Bit', async () => {
    const {asm, warn} = await transpileSource(` state { u32 TEST_CONST; }
      function test_scalar_store()
      {
        u32<$t0> val, dst;

        store(val, dst);
        store(val, dst, 0x10);
        store(val, dst, TEST_CONST);
        
        store(val, TEST_CONST);
        store(val, TEST_CONST, 0x10);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_scalar_store:
  sw $t0, ($t1)
  sw $t0, 16($t1)
  sw $t0, %lo(TEST_CONST)($t1)
  sw $t0, %lo(TEST_CONST)($zero)
  sw $t0, %lo(16 + TEST_CONST)($zero)
  jr $ra
  nop`);
  });

  test('Scalar - Cast', async () => {
    const {asm, warn} = await transpileSource(` state { u32 TEST_CONST; }
      function test_scalar_store()
      {
        u32<$t0> val, dst;

        store(val:u32, dst);
        store(val:u16, dst);
        store(val:u8, dst);
        
        store(val:s32, dst);
        store(val:s16, dst);
        store(val:s8, dst);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_scalar_store:
  sw $t0, ($t1)
  sh $t0, ($t1)
  sb $t0, ($t1)
  sw $t0, ($t1)
  sh $t0, ($t1)
  sb $t0, ($t1)
  jr $ra
  nop`);
  });

  test('Vector - 32-Bit', async () => {
    const {asm, warn} = await transpileSource(` state { u32 TEST_CONST; }
      function test_vector_store() 
      {
        u32<$t0> dst;
        vec32<$v01> val;
        
        WholeVector:
        store(val, dst);
        store(val, TEST_CONST);
        
        Swizzle:
        store(val.y, dst);
        store(val.z, dst, 0x10);
        store(val.zw, dst);
        store(val.zw, dst, 0x10);
        store(val.XYZW, dst);
        store(val.XYZW, dst, 0x10);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_vector_store:
  WholeVector:
  sqv $v01, 0, 0, $t0
  sqv $v02, 0, 16, $t0
  ori $at, $zero, %lo(TEST_CONST)
  sqv $v01, 0, 0, $at
  sqv $v02, 0, 16, $at
  Swizzle:
  ssv $v01, 2, 0, $t0
  ssv $v02, 2, 2, $t0
  ssv $v01, 4, 16, $t0
  ssv $v02, 4, 18, $t0
  slv $v01, 4, 0, $t0
  slv $v02, 4, 4, $t0
  slv $v01, 4, 16, $t0
  slv $v02, 4, 20, $t0
  sdv $v01, 8, 0, $t0
  sdv $v02, 8, 8, $t0
  sdv $v01, 8, 16, $t0
  sdv $v02, 8, 24, $t0
  jr $ra
  nop`);
  });

  test('Vector - Cast', async () => {
    const {asm, warn} = await transpileSource(` state { u32 TEST_CONST; }
      function test_vector_store() 
      {
        u32<$t0> dst;
        vec32<$v01> val;
        
        WholeVector:
        store(val:uint, dst);
        store(val:ufract, dst);
        
        Swizzle:
        store(val:uint.XYZW, dst);
        store(val:ufract.XYZW, dst);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_vector_store:
  WholeVector:
  sqv $v01, 0, 0, $t0
  sqv $v02, 0, 0, $t0
  Swizzle:
  sdv $v01, 8, 0, $t0
  sdv $v02, 8, 0, $t0
  jr $ra
  nop`);
  });

 test('Vector - Packed-Store', async () => {
    const {asm, warn} = await transpileSource(`function test() 
      {
        u32<$t0> dst;
        vec16<$v01> val;
        
        Unsigned:
        store_vec_u8(val, dst);
        store_vec_u8(val, dst, 0x10);
        store_vec_u8(val.y, dst);
        store_vec_u8(val.z, dst, 0x10);
        
        Signed:
        store_vec_s8(val, dst);
        store_vec_s8(val, dst, 0x10);
        store_vec_s8(val.y, dst);
        store_vec_s8(val.z, dst, 0x10);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  Unsigned:
  suv $v01, 0, 0, $t0
  suv $v01, 0, 16, $t0
  suv $v01, 1, 0, $t0
  suv $v01, 2, 16, $t0
  Signed:
  spv $v01, 0, 0, $t0
  spv $v01, 0, 16, $t0
  spv $v01, 1, 0, $t0
  spv $v01, 2, 16, $t0
  jr $ra
  nop`);
  });

    test('Vector - Transposed-Store', async () => {
    const {asm, warn} = await transpileSource(`function test() 
      {
        u32<$t0> ptr;
        vec16<$v08> a;
        vec16<$v16> b;
        
        store_transposed(a, 0, ptr, 0x00);
        store_transposed(a, 0, ptr);
        store_transposed(a, 1, ptr, 0x10);
        store_transposed(b, 4, ptr, 0x20);
        store_transposed(b, 7, ptr, 0x30);
        END:
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  stv $v08, 0, 0, $t0
  stv $v08, 0, 0, $t0
  stv $v08, 2, 16, $t0
  stv $v16, 8, 32, $t0
  stv $v16, 14, 48, $t0
  END:
  jr $ra
  nop`);
  });

  test('Invalid Transpose Store - reg', async () => {
    const src = `function test() {
      u32<$t0> ptr;
      vec32<$v04> v;
      store_transposed(v, 0, ptr, 0x00);
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/Error in test, line 4: Builtin store_transposed\(\) requires target register to be \$v00, \$v08, \$v16 or \$v24!/);
  });

  test('Invalid Transpose Store - offset', async () => {
    const src = `function test() {
      u32<$t0> ptr;
      vec32<$v16> v;
      store_transposed(v, 0, ptr, 0x04);
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/Error in test, line 4: Builtin store_transposed\(\) requires offset to be multiple of 16/);
  });
});
import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Store', () =>
{
  test('Scalar - 32-Bit', () => {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
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
  sw $t0, 16 + %lo(TEST_CONST)($zero)
  jr $ra
  nop`);
  });

  test('Scalar - Cast', () => {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
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

  test('Vector - 32-Bit', () => {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
      function test_vector_store() 
      {
        u32<$t0> dst;
        vec32<$v01> val;
        
        // Whole Vector
        store(val, dst);
        store(val, TEST_CONST);
        
        // Swizzle
        store(val.y, dst);
        store(val.z, dst, 0x10);
        store(val.zw, dst);
        store(val.zw, dst, 0x10);
        store(val.XYZW, dst);
        store(val.XYZW, dst, 0x10);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_vector_store:
  ## Whole Vector
  sqv $v01, 0, 0, $t0
  sqv $v02, 0, 16, $t0
  ori $at, $zero, %lo(TEST_CONST)
  sqv $v01, 0, 0, $at
  sqv $v02, 0, 16, $at
  ## Swizzle
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

  test('Vector - Cast', () => {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
      function test_vector_store() 
      {
        u32<$t0> dst;
        vec32<$v01> val;
        
        // Whole Vector
        store(val:int, dst);
        store(val:fract, dst);
        
        // Swizzle
        store(val:int.XYZW, dst);
        store(val:fract.XYZW, dst);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_vector_store:
  ## Whole Vector
  sqv $v01, 0, 0, $t0
  sqv $v02, 0, 0, $t0
  ## Swizzle
  sdv $v01, 8, 0, $t0
  sdv $v02, 8, 0, $t0
  jr $ra
  nop`);
  });

 test('Vector - Packed-Store', () => {
    const {asm, warn} = transpileSource(`function test() 
      {
        u32<$t0> dst;
        vec16<$v01> val;
        
        // Unsigned
        store_vec_u8(val, dst);
        store_vec_u8(val, dst, 0x10);
        store_vec_u8(val.y, dst);
        store_vec_u8(val.z, dst, 0x10);
        
        // Signed
        store_vec_s8(val, dst);
        store_vec_s8(val, dst, 0x10);
        store_vec_s8(val.y, dst);
        store_vec_s8(val.z, dst, 0x10);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ## Unsigned
  suv $v01, 0, 0, $t0
  suv $v01, 0, 16, $t0
  suv $v01, 1, 0, $t0
  suv $v01, 2, 16, $t0
  ## Signed
  spv $v01, 0, 0, $t0
  spv $v01, 0, 16, $t0
  spv $v01, 1, 0, $t0
  spv $v01, 2, 16, $t0
  jr $ra
  nop`);
  });
});
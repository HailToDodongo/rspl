import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Load', () =>
{
  test('Scalar - 32-Bit', () => {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
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
  ## dst = load(TEST_CONST, TEST_CONST); Invalid
  jr $ra
  nop`);
  });

  test('Vector - 32-Bit', () => {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
      function test_vector_load() 
      {
        u32<$t0> src;
        vec32<$v01> dst;
        // Whole Vector
        dst = load(src);
        dst = load(src, 0x10);
        dst.y = load(src);
        dst.z = load(src, 0x10);
        //dst = load(src, TEST_CONST); Invalid
        //dst = load(TEST_CONST); Invalid
        //dst = load(TEST_CONST, 0x10); Invalid
        
        // Swizzle
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
  ## Whole Vector
  lqv $v01, 0, 0, $t0
  lqv $v02, 0, 16, $t0
  lqv $v01, 0, 16, $t0
  lqv $v02, 0, 32, $t0
  lqv $v01, 2, 0, $t0
  lqv $v02, 2, 16, $t0
  lqv $v01, 4, 16, $t0
  lqv $v02, 4, 32, $t0
  ##dst = load(src, TEST_CONST); Invalid
  ##dst = load(TEST_CONST); Invalid
  ##dst = load(TEST_CONST, 0x10); Invalid
  ## Swizzle
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
  ##dst = load(src, TEST_CONST).xyzwxyzw; Invalid
  ##dst = load(TEST_CONST).xyzwxyzw; Invalid
  ##dst = load(TEST_CONST, 0x10).xyzwxyzw; Invalid
  jr $ra
  nop`);
  });

  test('Vector - 32-Bit Split', () => {
    const {asm, warn} = transpileSource(`function test() 
      {
        u32<$t0> src;
        vec32<$v01> dst;
        
        // Left-Side
        dst.xyzw = load(src, 0x00).xyzw;
        dst.xyzw = load(src, 0x10).xyzw;
        
        // Right-Side
        dst.XYZW = load(src, 0x00).XYZW;
        dst.XYZW = load(src, 0x10).XYZW;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ## Left-Side
  ldv $v01, 0, 0, $t0
  ldv $v02, 0, 8, $t0
  ldv $v01, 0, 16, $t0
  ldv $v02, 0, 24, $t0
  ## Right-Side
  ldv $v01, 8, 8, $t0
  ldv $v02, 8, 16, $t0
  ldv $v01, 8, 24, $t0
  ldv $v02, 8, 32, $t0
  jr $ra
  nop`);
  });

 test('Vector - Packed-Load', () => {
    const {asm, warn} = transpileSource(`function test() 
      {
        u32<$t0> src;
        vec16<$v01> dst;
        
        // Unsigned
        dst = load_vec_u8(src, 0x00);
        dst.z = load_vec_u8(src, 0x10);
        
        // Signed
        dst.x = load_vec_s8(src, 0x00);
        dst.z = load_vec_s8(src, 0x10);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ## Unsigned
  luv $v01, 0, 0, $t0
  luv $v01, 2, 16, $t0
  ## Signed
  lpv $v01, 0, 0, $t0
  lpv $v01, 2, 16, $t0
  jr $ra
  nop`);
  });
});
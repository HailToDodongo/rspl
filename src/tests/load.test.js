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
  jr $ra
  nop`);
  });

  test('Scalar - Cast', () => {
    const {asm, warn} = transpileSource(`
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

  test('Vector - 32-Bit', () => {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
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

  test('Vector - 32-Bit Split', () => {
    const {asm, warn} = transpileSource(`function test() 
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

  test('Vector - Cast', () => {
    const {asm, warn} = transpileSource(`function test() 
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

 test('Vector - Packed-Load', () => {
    const {asm, warn} = transpileSource(`function test() 
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
});
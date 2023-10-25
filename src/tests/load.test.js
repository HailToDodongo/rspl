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
  lw t1, 0(t0)
  lw t1, 16(t0)
  lw t1, %lo(TEST_CONST)(t0)
  lw t1, %lo(TEST_CONST + 0)
  lw t1, %lo(TEST_CONST + 16)
  ## dst = load(TEST_CONST, TEST_CONST); Invalid
  jr ra`);
  });

    test('Vector - 32-Bit', () => {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
      function test_vector_load() 
      {
        vec32<$t0> src;
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
  lqv $v01, 0x00, 0, t0
  lqv $v02, 0x00, 0 + 0x10, t0
  lqv $v01, 0x00, 16, t0
  lqv $v02, 0x00, 16 + 0x10, t0
  lqv $v01, 0x02, 0, t0
  lqv $v02, 0x02, 0 + 0x10, t0
  lqv $v01, 0x04, 16, t0
  lqv $v02, 0x04, 16 + 0x10, t0
  ##dst = load(src, TEST_CONST); Invalid
  ##dst = load(TEST_CONST); Invalid
  ##dst = load(TEST_CONST, 0x10); Invalid
  ## Swizzle
  ldv $v01, 0x00, 0, t0
  ldv $v01, 0x08, 0, t0
  ldv $v02, 0x00, 0 + 0x10, t0
  ldv $v02, 0x08, 0 + 0x10, t0
  ldv $v01, 0x00, 16, t0
  ldv $v01, 0x08, 16, t0
  ldv $v02, 0x00, 16 + 0x10, t0
  ldv $v02, 0x08, 16 + 0x10, t0
  ldv $v01, 0x02, 0, t0
  ldv $v01, 0x0A, 0, t0
  ldv $v02, 0x02, 0 + 0x10, t0
  ldv $v02, 0x0A, 0 + 0x10, t0
  ldv $v01, 0x04, 16, t0
  ldv $v01, 0x0C, 16, t0
  ldv $v02, 0x04, 16 + 0x10, t0
  ldv $v02, 0x0C, 16 + 0x10, t0
  ##dst = load(src, TEST_CONST).xyzwxyzw; Invalid
  ##dst = load(TEST_CONST).xyzwxyzw; Invalid
  ##dst = load(TEST_CONST, 0x10).xyzwxyzw; Invalid
  jr ra`);
  });
});
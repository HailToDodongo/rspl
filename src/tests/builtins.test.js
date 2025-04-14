import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Builtins', () =>
{
  test('swap() - scalar', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        u32 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  xor $t0, $t0, $t1
  xor $t1, $t0, $t1
  xor $t0, $t0, $t1
  jr $ra
  nop`);
  });

  test('swap() - vec16', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec16 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v01, $v01, $v02
  vxor $v02, $v01, $v02
  vxor $v01, $v01, $v02
  jr $ra
  nop`);
  });

  test('swap() - vec32', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec32 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v01, $v01, $v03
  vxor $v03, $v01, $v03
  vxor $v01, $v01, $v03
  vxor $v02, $v02, $v04
  vxor $v04, $v02, $v04
  vxor $v02, $v02, $v04
  jr $ra
  nop`);
  });

  test('asm_include()', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        u32 a = 4;
        asm_include("./test.inc");
        a = 5;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $t0, $zero, 4
  #define zero $0
  #define v0 $2
  #define v1 $3
  #define a0 $4
  #define a1 $5
  #define a2 $6
  #define a3 $7
  #define t0 $8
  #define t1 $9
  #define t2 $10
  #define t3 $11
  #define t4 $12
  #define t5 $13
  #define t6 $14
  #define t7 $15
  #define s0 $16
  #define s1 $17
  #define s2 $18
  #define s3 $19
  #define s4 $20
  #define s5 $21
  #define s6 $22
  #define s7 $23
  #define t8 $24
  #define t9 $25
  #define k0 $26
  #define k1 $27
  #define gp $28
  #define sp $29
  #define fp $30
  #define ra $31
  .set at
  .set macro
  #include "./test.inc"
  .set noreorder
  .set noat
  .set nomacro
  #undef zero
  #undef at
  #undef v0
  #undef v1
  #undef a0
  #undef a1
  #undef a2
  #undef a3
  #undef t0
  #undef t1
  #undef t2
  #undef t3
  #undef t4
  #undef t5
  #undef t6
  #undef t7
  #undef s0
  #undef s1
  #undef s2
  #undef s3
  #undef s4
  #undef s5
  #undef s6
  #undef s7
  #undef t8
  #undef t9
  #undef k0
  #undef k1
  #undef gp
  #undef sp
  #undef fp
  #undef ra
  addiu $t0, $zero, 5
  jr $ra
  nop`);
  });
});
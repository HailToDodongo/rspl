import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Scalar - Ops', () =>
{
  test('32-Bit Arithmetic', async () => {
    const {asm, warn} = await transpileSource(`state { u32 TEST_CONST; }
      function test_scalar_ops()
      {
        u32<$t0> a, b, c;
        s32<$t3> sa, sb, sc;
        
        ADD:
        c = a + b; sc = sa + sb;
        c = a + 1; sc = sa + 1;
        c = a + TEST_CONST; sc = sa + TEST_CONST;
                
        SUB:
        c = a - b; sc = sa - sb;
        c = a - 1; sc = sa - 1;
        //c = a - TEST_CONST; sc = sa - TEST_CONST; Invalid
        
        MUL:
        c = a * 4;
        
        DIV:
        c = a / 8;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(
`test_scalar_ops:
  ADD:
  addu $t2, $t0, $t1
  addu $t5, $t3, $t4
  addiu $t2, $t0, 1
  addiu $t5, $t3, 1
  addiu $t2, $t0, %lo(TEST_CONST)
  addiu $t5, $t3, %lo(TEST_CONST)
  SUB:
  subu $t2, $t0, $t1
  subu $t5, $t3, $t4
  addiu $t2, $t0, 65535
  addiu $t5, $t3, 65535
  MUL:
  sll $t2, $t0, 2
  DIV:
  srl $t2, $t0, 3
  jr $ra
  nop`);
  });
  
  test('32-Bit - Logic', async () => {
    const {asm, warn} = await transpileSource(`state { u32 TEST_CONST; }
      function test_scalar_ops()
      {
        u32<$t0> a, b, c;
        s32<$t3> sa, sb, sc;
                
        AND:
        c = a & b;
        c = a & 1;
        c = a & TEST_CONST;
        
        OR:
        c = a | b;
        c = a | 2;
        c = a | TEST_CONST;
        
        XOR:
        c = a ^ b;
        c = a ^ 2;
        c = a ^ TEST_CONST;
        
        NOT:
        c = ~b;
        
        NOR:
        c = a ~| b;
        
        SHIFT_LEFT:
        c = a << b;
        c = a << 2;
        //c = a << TEST_CONST; Invalid
        
        SHIFT_RIGHT:
        c = a >> b;
        c = a >> 2;
        //c = a >> TEST_CONST; Invalid
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(
`test_scalar_ops:
  AND:
  and $t2, $t0, $t1
  andi $t2, $t0, 1
  andi $t2, $t0, %lo(TEST_CONST)
  OR:
  or $t2, $t0, $t1
  ori $t2, $t0, 2
  ori $t2, $t0, %lo(TEST_CONST)
  XOR:
  xor $t2, $t0, $t1
  xori $t2, $t0, 2
  xori $t2, $t0, %lo(TEST_CONST)
  NOT:
  nor $t2, $zero, $t1
  NOR:
  nor $t2, $t0, $t1
  SHIFT_LEFT:
  sllv $t2, $t0, $t1
  sll $t2, $t0, 2
  SHIFT_RIGHT:
  srlv $t2, $t0, $t1
  srl $t2, $t0, 2
  jr $ra
  nop`);
  });

  test('Multiplication (2^x)', async () => {
    const src = `function test() {
      u32<$t0> a, b;
      a = b * 4;
    }`;

    const {asm, warn} = await transpileSource(src, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  sll $t0, $t1, 2
  jr $ra
  nop`);
  });

  test('Division (2^x)', async () => {
    const src = `function test() {
      u32<$t0> a, b;
      a = b / 8;
    }`;

    const {asm, warn} = await transpileSource(src, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  srl $t0, $t1, 3
  jr $ra
  nop`);
  });

  test('Assign - scalar', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        u32<$t0> a;
        u32<$t1> b = a;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t1, $zero, $t0
  jr $ra
  nop`);
  });

  test('Assign - Vector (ufract)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec32 v0;
        vec16 v1;
        u32 a = v0:ufract.y;
        u32 b = v1:ufract.y;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mfc2 $t0, $v02.e1
  mfc2 $t1, $v03.e1
  jr $ra
  nop`);
  });

  test('Assign - Vector (sint)', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec32 v0;
        vec16 v1;
        u32 a = v0:sint.y;
        u32 b = v1:sint.y;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mfc2 $t0, $v01.e1
  mfc2 $t1, $v03.e1
  jr $ra
  nop`);
  });

  test('Invalid (multiplication)', async () => {
    const src = `function test() {
      u32<$t0> a, b;
      a = a * b;
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 3: Scalar-Multiplication only allowed with a power-of-two /);
  });

  test('Invalid (division)', async () => {
    const src = `function test() {
      u32<$t0> a, b;
      a = a / b;
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 3: Scalar-Division only allowed with a power-of-two /);
  });

  test('Invalid (sub with label)', async () => {
    const src = `state { u32 TEST_CONST; }
    function test() {
      u32<$t0> a;
      a = a - TEST_CONST;
    }`;

   await expect(() => transpileSource(src, CONF))
    .rejects.toThrowError(/line 4: Subtraction cannot use labels!/);
  });
});
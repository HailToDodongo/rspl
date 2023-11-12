import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Scalar - Ops', () =>
{
  test('32-Bit Arithmetic', () => {
    const {asm, warn} = transpileSource(`state { u32 TEST_CONST; }
      function test_scalar_ops()
      {
        u32<$t0> a, b, c;
        s32<$t3> sa, sb, sc;
        
        // Add
        c = a + b; sc = sa + sb;
        c = a + 1; sc = sa + 1;
        c = a + TEST_CONST; sc = sa + TEST_CONST;
                
        // Sub
        c = a - b; sc = sa - sb;
        c = a - 1; sc = sa - 1;
        //c = a - TEST_CONST; sc = sa - TEST_CONST; Invalid
        
        // Mul/Div not possible
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(
`test_scalar_ops:
  ## Add
  addu $t2, $t0, $t1
  addu $t5, $t3, $t4
  addiu $t2, $t0, 1
  addiu $t5, $t3, 1
  addiu $t2, $t0, %lo(TEST_CONST)
  addiu $t5, $t3, %lo(TEST_CONST)
  ## Sub
  subu $t2, $t0, $t1
  subu $t5, $t3, $t4
  addiu $t2, $t0, 65535
  addiu $t5, $t3, 65535
  ##c = a - TEST_CONST; sc = sa - TEST_CONST; Invalid
  ## Mul/Div not possible
  jr $ra
  nop`);
  });
  
  test('32-Bit - Logic', () => {
    const {asm, warn} = transpileSource(`state { u32 TEST_CONST; }
      function test_scalar_ops()
      {
        u32<$t0> a, b, c;
        s32<$t3> sa, sb, sc;
                
        // And
        c = a & b;
        c = a & 1;
        c = a & TEST_CONST;
        
        // Or
        c = a | b;
        c = a | 2;
        c = a | TEST_CONST;
        
        // XOR
        c = a ^ b;
        c = a ^ 2;
        c = a ^ TEST_CONST;
        
        // Not
        c = ~b;
        
        // Shift-Left
        c = a << b;
        c = a << 2;
        //c = a << TEST_CONST; Invalid
        
        // Shift-Right
        c = a >> b;
        c = a >> 2;
        //c = a >> TEST_CONST; Invalid
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(
`test_scalar_ops:
  ## And
  and $t2, $t0, $t1
  andi $t2, $t0, 1
  andi $t2, $t0, %lo(TEST_CONST)
  ## Or
  or $t2, $t0, $t1
  ori $t2, $t0, 2
  ori $t2, $t0, %lo(TEST_CONST)
  ## XOR
  xor $t2, $t0, $t1
  xori $t2, $t0, 2
  xori $t2, $t0, %lo(TEST_CONST)
  ## Not
  nor $t2, $zero, $t1
  ## Shift-Left
  sllv $t2, $t0, $t1
  sll $t2, $t0, 2
  ##c = a << TEST_CONST; Invalid
  ## Shift-Right
  srlv $t2, $t0, $t1
  srl $t2, $t0, 2
  ##c = a >> TEST_CONST; Invalid
  jr $ra
  nop`);
  });

  test('Multiplication (2^x)', () => {
    const src = `function test() {
      u32<$t0> a, b;
      a = b * 4;
    }`;

    const {asm, warn} = transpileSource(src, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  sll $t0, $t1, 2
  jr $ra
  nop`);
  });

  test('Division (2^x)', () => {
    const src = `function test() {
      u32<$t0> a, b;
      a = b / 8;
    }`;

    const {asm, warn} = transpileSource(src, CONF);
    expect(warn).toBe("");
    expect(asm).toBe(`test:
  srl $t0, $t1, 3
  jr $ra
  nop`);
  });

  test('Assign - scalar', () => {
    const {asm, warn} = transpileSource(`function test() {
        u32<$t0> a;
        u32<$t1> b = a;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t1, $zero, $t0
  jr $ra
  nop`);
  });

  test('Assign - Vector (fract)', () => {
    const {asm, warn} = transpileSource(`function test() {
        vec32 v0;
        vec16 v1;
        u32 a = v0:fract.y;
        u32 b = v1:fract.y;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mfc2 $t0, $v02.e1
  mfc2 $t1, $v03.e1
  jr $ra
  nop`);
  });

  test('Assign - Vector (int)', () => {
    const {asm, warn} = transpileSource(`function test() {
        vec32 v0;
        vec16 v1;
        u32 a = v0:int.y;
        u32 b = v1:int.y;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mfc2 $t0, $v01.e1
  mfc2 $t1, $v03.e1
  jr $ra
  nop`);
  });

  test('Invalid (multiplication)', () => {
    const src = `function test() {
      u32<$t0> a, b;
      a = a * b;
    }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 3: Scalar-Multiplication only allowed with a power-of-two /);
  });

  test('Invalid (division)', () => {
    const src = `function test() {
      u32<$t0> a, b;
      a = a / b;
    }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 3: Scalar-Division only allowed with a power-of-two /);
  });

  test('Invalid (sub with label)', () => {
    const src = `state { u32 TEST_CONST; }
    function test() {
      u32<$t0> a;
      a = a - TEST_CONST;
    }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 4: Subtraction cannot use labels!/);
  });
});
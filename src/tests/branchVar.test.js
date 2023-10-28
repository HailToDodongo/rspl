import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Branch (Var vs. Var)', () =>
{
  test('Equal - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if(a == b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne $v0, $v1, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Not-Equal - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if(a != b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq $v0, $v1, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Greater - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if(a > b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  sltu $at, $v1, $v0
  beq $at, $zero, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Less - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if(a < b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  sltu $at, $v0, $v1
  beq $at, $zero, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

    test('Greater-Than - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if(a >= b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  sltu $at, $v0, $v1
  bne $at, $zero, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Less-Than - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if(a <= b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  sltu $at, $v1, $v0
  bne $at, $zero, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  // Signed

  test('Equal - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if(a == b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne $v0, $v1, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Not-Equal - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if(a != b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq $v0, $v1, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Greater - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if(a > b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  slt $at, $v1, $v0
  beq $at, $zero, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Less - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if(a < b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  slt $at, $v0, $v1
  beq $at, $zero, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Greater-Than - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if(a >= b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  slt $at, $v0, $v1
  bne $at, $zero, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });

  test('Less-Than - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if(a <= b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  slt $at, $v1, $v0
  bne $at, $zero, 1f
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, 2f
  nop
  1:
  addiu $v0, $v0, 2222
  2:
  jr $ra
  nop`);
  });
});
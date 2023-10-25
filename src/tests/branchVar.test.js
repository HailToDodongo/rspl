import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Branch (Var vs. Var)', () =>
{
  test('Equal - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if<$t1>(a == b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne v0, v1, 1f
  nop
  addiu v0, v0, 1111
  b 2f
  nop
  1:
  addiu v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Not-Equal - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if<$t1>(a != b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq v0, v1, 1f
  nop
  addiu v0, v0, 1111
  b 2f
  nop
  1:
  addiu v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Greater - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if<$t1>(a > b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq v1, zero, 1f
  sltu v1, v1, v0
  addiu v0, v0, 1111
  b 2f
  nop
  1:
  addiu v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Less - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if<$t1>(a < b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq v1, zero, 1f
  sltu v1, v0, v1
  addiu v0, v0, 1111
  b 2f
  nop
  1:
  addiu v0, v0, 2222
  2:
  jr ra
  nop`);
  });

    test('Greater-Than - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if<$t1>(a >= b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne v1, zero, 1f
  sltu v1, v1, v0
  addiu v0, v0, 1111
  b 2f
  nop
  1:
  addiu v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Less-Than - U32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      u32<$v0> a,b;
      if<$t1>(a <= b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne v1, zero, 1f
  sltu v1, v1, v0
  addiu v0, v0, 1111
  b 2f
  nop
  1:
  addiu v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  // Signed

  test('Equal - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if<$t1>(a == b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne v0, v1, 1f
  nop
  addi v0, v0, 1111
  b 2f
  nop
  1:
  addi v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Not-Equal - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if<$t1>(a != b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq v0, v1, 1f
  nop
  addi v0, v0, 1111
  b 2f
  nop
  1:
  addi v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Greater - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if<$t1>(a > b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq v1, zero, 1f
  sltu v1, v1, v0
  addi v0, v0, 1111
  b 2f
  nop
  1:
  addi v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Less - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if<$t1>(a < b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq v1, zero, 1f
  sltu v1, v0, v1
  addi v0, v0, 1111
  b 2f
  nop
  1:
  addi v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Greater-Than - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if<$t1>(a >= b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne v1, zero, 1f
  sltu v1, v1, v0
  addi v0, v0, 1111
  b 2f
  nop
  1:
  addi v0, v0, 2222
  2:
  jr ra
  nop`);
  });

  test('Less-Than - S32', () => {
    const {asm, warn} = transpileSource(`function test_if() {
      s32<$v0> a,b;
      if<$t1>(a <= b) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne v1, zero, 1f
  sltu v1, v1, v0
  addi v0, v0, 1111
  b 2f
  nop
  1:
  addi v0, v0, 2222
  2:
  jr ra
  nop`);
  });
});
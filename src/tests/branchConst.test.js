import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Branch (Var vs. Const)', () =>
{
  test('Equal - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a == 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  addiu $at, $zero, 42
  bne $v0, $at, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

    test('Equal - U32 (big number)', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a == 0x112233) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  lui $at, 0x11
  ori $at, $at, 0x2233
  bne $v0, $at, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Not-Equal - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a != 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  addiu $at, $zero, 42
  beq $v0, $at, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Greater - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a > 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  sltiu $at, $v0, 43
  bne $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Greater - U32 (big number)', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a > 0xFFFEFFFF) { a += 1111; } else { a += 2222; }
    }`, CONF);

    // 0xFFFEFFFF turns into 0xFFFF0000 after inverting to compare, short load can be used here then
    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  lui $at, 0xFFFF
  sltu $at, $v0, $at
  bne $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Less - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a < 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  sltiu $at, $v0, 42
  beq $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

    test('Greater-Than - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a >= 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  sltiu $at, $v0, 42
  bne $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Less-Than - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a <= 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  sltiu $at, $v0, 43
  beq $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  // Signed

  test('Equal - S32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      s32<$v0> a;
      if(a == 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  addiu $at, $zero, 42
  bne $v0, $at, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Not-Equal - S32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      s32<$v0> a;
      if(a != 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  addiu $at, $zero, 42
  beq $v0, $at, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Greater - S32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      s32<$v0> a;
      if(a > 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  slti $at, $v0, 43
  bne $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Less - S32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      s32<$v0> a;
      if(a < 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  slti $at, $v0, 42
  beq $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Greater-Than - S32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      s32<$v0> a;
      if(a >= 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  slti $at, $v0, 42
  bne $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });

  test('Less-Than - S32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      s32<$v0> a;
      if(a <= 42) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  slti $at, $v0, 43
  beq $at, $zero, LABEL_test_if_0001
  nop
  addiu $v0, $v0, 1111
  beq $zero, $zero, LABEL_test_if_0002
  nop
  LABEL_test_if_0001:
  addiu $v0, $v0, 2222
  LABEL_test_if_0002:
  jr $ra
  nop`);
  });
});
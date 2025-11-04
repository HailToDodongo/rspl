import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Branch (Var vs. 0)', () =>
{
  test('Equal - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a == 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne $v0, $zero, LABEL_test_if_0001
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
      if(a != 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq $v0, $zero, LABEL_test_if_0001
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
      if(a > 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  blez $v0, LABEL_test_if_0001
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
      if(a < 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bgez $v0, LABEL_test_if_0001
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

    test('Greater-Equal - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a >= 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bltz $v0, LABEL_test_if_0001
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

  test('Less-Equal - U32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      u32<$v0> a;
      if(a <= 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bgtz $v0, LABEL_test_if_0001
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
      if(a == 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bne $v0, $zero, LABEL_test_if_0001
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
      if(a != 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  beq $v0, $zero, LABEL_test_if_0001
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
      if(a > 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  blez $v0, LABEL_test_if_0001
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
      if(a < 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bgez $v0, LABEL_test_if_0001
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

  test('Greater-Equal - S32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      s32<$v0> a;
      if(a >= 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bltz $v0, LABEL_test_if_0001
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

  test('Less-Equal - S32', async () => {
    const {asm, warn} = await transpileSource(`function test_if() {
      s32<$v0> a;
      if(a <= 0) { a += 1111; } else { a += 2222; }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_if:
  bgtz $v0, LABEL_test_if_0001
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
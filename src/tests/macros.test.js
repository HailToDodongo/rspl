import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Macros', () =>
{
  test('Basic replacement', async () => {
    const {asm, warn} = await transpileSource(`
      macro test(u32 add) {
        add += 42;
      }
      
      function test_macro() {
        u32<$t2> a;
        u32<$s3> b;
        test(a);
        
        if(a < 3) {
          test(a);
        }
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_macro:
  addiu $t2, $t2, 42
  sltiu $at, $t2, 3
  beq $at, $zero, LABEL_test_macro_0001
  nop
  addiu $t2, $t2, 42
  LABEL_test_macro_0001:
  jr $ra
  nop`);
  });

  test('Nested macro', async () => {
    const {asm, warn} = await transpileSource(`
      macro test_b(u32 argB) {
        argB += 42;
      }
      
      macro test_a(u32 argA) {
        test_b(argA);
      }
      
      function test_macro() {
        u32<$t2> a;
        test_a(a);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_macro:
  addiu $t2, $t2, 42
  jr $ra
  nop`);
  });

  test('Scope local', async () => {
    const {asm, warn} = await transpileSource(`
      macro test_b(u32 argB) {
        argB += 42;
      }
      
      function test_macro() {
        u32<$t2> a;
        u32<$t3> argB;
        test_b(a);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_macro:
  addiu $t2, $t2, 42
  jr $ra
  nop`);
  });

    test('Return Value', async () => {
    const {asm, warn} = await transpileSource(`
      macro test_a(u32 res, u32 argA, u32 argB) {
        res = argA + argB;
      }
      
      function test_macro() {
        u32<$a0> argA, argB;
        u32<$s0> a = test_a(argA, argB);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_macro:
  addu $s0, $a0, $a1
  jr $ra
  nop`);
  });

  test('Local basic', async () => {
    const {asm, warn} = await transpileSource(`
      function test_local() {
        macro inc(u32 add) {
          add += 42;
        }
        u32<$t2> a;
        inc(a);

        if(a < 3) {
          inc(a);
        }
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_local:
  addiu $t2, $t2, 42
  sltiu $at, $t2, 3
  beq $at, $zero, LABEL_test_local_0001
  nop
  addiu $t2, $t2, 42
  LABEL_test_local_0001:
  jr $ra
  nop`);
  });

  test('Local sees call-site scope', async () => {

    const {asm, warn} = await transpileSource(`
      function test_capture() {
        u32<$t0> base;
        macro addBoth(u32 dst) {
          dst += base;
          dst += later;
        }
        u32<$t1> later;
        u32<$t2> a;
        addBoth(a);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_capture:
  addu $t2, $t2, $t0
  addu $t2, $t2, $t1
  jr $ra
  nop`);
  });

  test('Local de-phasing (multiple inlinings)', async () => {
    const {asm, warn} = await transpileSource(`
      function test_dephase() {
        macro step(u32 v) {
          v += 1;
        }
        u32<$t0> a;
        u32<$t1> b;
        step(a);
        step(b);
        step(a);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_dephase:
  addiu $t0, $t0, 1
  addiu $t1, $t1, 1
  addiu $t0, $t0, 1
  jr $ra
  nop`);
  });

  test('Local shadows global', async () => {
    const {asm, warn} = await transpileSource(`
      macro doOp(u32 v) {
        v += 1;
      }

      function test_shadow() {
        u32<$t0> a;
        doOp(a);
        macro doOp(u32 v) {
          v += 2;
        }
        doOp(a);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_shadow:
  addiu $t0, $t0, 1
  addiu $t0, $t0, 2
  jr $ra
  nop`);
  });

  test('Local return value', async () => {
    const {asm, warn} = await transpileSource(`
      function test_ret() {
        macro sum(u32 res, u32 x, u32 y) {
          res = x + y;
        }
        u32<$a0> argA, argB;
        u32<$s0> a = sum(argA, argB);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_ret:
  addu $s0, $a0, $a1
  jr $ra
  nop`);
  });

  test('Local dies with enclosing block', async () => {
    const src = `
      function test_scope_end() {
        u32<$t0> a;
        {
          macro inc(u32 v) { v += 1; }
          inc(a);
        }
        inc(a);
      }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Function inc not known/);
  });

  test('Local not visible before declaration', async () => {
    const src = `
      function test_before() {
        u32<$t0> a;
        inc(a);
        macro inc(u32 v) { v += 1; }
      }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Function inc not known/);
  });

  test('Local parse errors', async () => {
    // only macros may be nested
    await expect(() => transpileSource(`function outer() {
                          function inner() {}
                        }`, CONF)).rejects.toThrow();
    // no result-type on macros
    await expect(() => transpileSource(`function outer() {
                          macro m<0x1>() {}
                        }`, CONF)).rejects.toThrow();
    // no forward declarations inside a function
    await expect(() => transpileSource(`function outer() {
                          macro m();
                        }`, CONF)).rejects.toThrow();
  });
});

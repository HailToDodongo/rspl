import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

// --- set_rsp_status ---

describe('Builtins - Debug', () =>
{
  test('set_rsp_status() - scalar', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32<$t0> a;
      set_rsp_status(a);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mtc0 $t0, COP0_SP_STATUS
  jr $ra
  nop`);
  });

  test('set_rsp_status() - scalar literal', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      set_rsp_status(42);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $at, $zero, 42
  mtc0 $at, COP0_SP_STATUS
  jr $ra
  nop`);
  });

  test('set_rsp_status() - fails with no argument', async () => {
    await expect(transpileSource(`function test() {
      set_rsp_status();
    }`, CONF)).rejects.toThrow("requires 1 scalar");
  });

  test('set_rsp_status() - fails with left side', async () => {
    await expect(transpileSource(`function test() {
      u32<$t0> a = set_rsp_status();
    }`, CONF)).rejects.toThrow("must not have a left side");
  });

  test('set_rsp_status() - fails with vector', async () => {
    await expect(transpileSource(`function test() {
      vec16<$v01> a;
      set_rsp_status(a);
    }`, CONF)).rejects.toThrow("scalar argument");
  });

  // --- print ---

  test('print() - scalar', async () => {
    const {asm, warn, info} = await transpileSource(`function test() {
      u32<$t0> a;
      u32<$t1> b;
      print(a, b);
    }`, CONF);

    expect(warn).toBe("");
    expect(info).toContain("print() variable");
    expect(asm).toBe(`test:
  .set macro # print
  xlogregs_gpr $t0, $t1
  .set noat # print
  .set nomacro # print
  jr $ra
  nop`);
  });

  test('print() - vector', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> a;
      vec16<$v03> b;
      print(a, b);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  .set macro # print
  xlogregs_vpr $v01, $v03
  .set noat # print
  .set nomacro # print
  jr $ra
  nop`);
  });

  test('print() - string', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      print("hello", "world");
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  .set macro # print
  emux_log_string "hello", "world"
  .set noat # print
  .set nomacro # print
  jr $ra
  nop`);
  });

  test('print() - fails with no arguments', async () => {
    await expect(transpileSource(`function test() {
      print();
    }`, CONF)).rejects.toThrow("requires at least one argument");
  });

  test('print() - fails with left side', async () => {
    await expect(transpileSource(`function test() {
      u32<$t0> a = print();
    }`, CONF)).rejects.toThrow("cannot have a left side");
  });

  test('print() - fails with mixed types', async () => {
    await expect(transpileSource(`function test() {
      u32<$t0> a;
      print(a, "hello");
    }`, CONF)).rejects.toThrow("same type");
  });

  test('print() - fails with number literal', async () => {
    await expect(transpileSource(`function test() {
      print(42);
    }`, CONF)).rejects.toThrow("variables or strings");
  });

  test('print() - fails with mixed scalar/vector', async () => {
    await expect(transpileSource(`function test() {
      u32<$t0> a;
      vec16<$v01> b;
      print(a, b);
    }`, CONF)).rejects.toThrow("mixed scalar/vector");
  });

  // --- printf ---

  test('printf() - basic scalar', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32<$t0> a;
      printf("hello %d", a);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  .set macro # print
  emux_printf "hello %dt0"
  .set noat # print
  .set nomacro # print
  jr $ra
  nop`);
  });

  test('printf() - vec32 with swizzle', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec32<$v01> a;
      printf("result %f", a.x);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toContain("emux_printf");
    expect(asm).toContain("result %f");
  });

  test('printf() - vec16', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v03> a;
      printf("val %d", a);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toContain("emux_printf");
    expect(asm).toContain("val %d");
  });

  test('printf() - string only', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      printf("hello world");
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toContain("emux_printf");
    expect(asm).toContain("hello world");
  });

  test('printf() - fails with no arguments', async () => {
    await expect(transpileSource(`function test() {
      printf();
    }`, CONF)).rejects.toThrow("requires at least one argument");
  });

  test('printf() - fails with left side', async () => {
    await expect(transpileSource(`function test() {
      u32<$t0> a = printf("hello");
    }`, CONF)).rejects.toThrow("cannot have a left side");
  });

  test('printf() - fails with non-string first arg', async () => {
    await expect(transpileSource(`function test() {
      u32<$t0> a;
      printf(a);
    }`, CONF)).rejects.toThrow("first argument to be a string");
  });
});
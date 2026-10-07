import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

async function requireThrowsWith(src, msgPart) {
  let threw = false;
  try {
    await transpileSource(src, CONF);
  } catch(e) {
    threw = true;
    expect(e.message).toContain(msgPart);
  }
  expect(threw).toBe(true);
}

describe('GlobalVars', () =>
{
  test('shared across functions', async () => {
    const {asm, warn} = await transpileSource(`
      u32<$k0> globCounter;

      function fnA() {
        globCounter += 1;
      }

      function fnB() {
        globCounter += 2;
      }`, CONF);

    expect(warn).toBe("");
    // same pinned register in both functions
    expect(asm).toContain("fnA:");
    expect(asm).toContain("fnB:");
    expect(asm).toContain("addiu $k0, $k0, 1");
    expect(asm).toContain("addiu $k0, $k0, 2");
  });

  test('excluded from auto-allocation', async () => {
    // $t0 is pinned, so the first auto-allocated scalar must be $t1
    const {asm, warn} = await transpileSource(`
      u32<$t0> pinned;

      function test() {
        u32 a;
        a += 1;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $t1, $t1, 1
  jr $ra
  nop`);
  });

  test('vector global usable in ops', async () => {
    // $v01 pinned -> auto-allocated vector goes to $v02
    const {asm, warn} = await transpileSource(`
      vec16<$v01> globVec;

      function test() {
        vec16 v;
        v = globVec +* globVec.x;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadn $v02, $v01, $v01.e0
  jr $ra
  nop`);
  });

  test('vec32 pins two registers', async () => {
    // vec32 on $v01 occupies $v01+$v02 -> auto-alloc starts at $v03
    const {asm, warn} = await transpileSource(`
      vec32<$v01> globVec;

      function test() {
        vec16 v;
        v = v +* v.x;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmadn $v03, $v03, $v03.e0
  jr $ra
  nop`);
  });

  test('undef is rejected', async () => {
    await requireThrowsWith(`
      u32<$t0> pinned;

      function test() {
        undef pinned;
      }`, "Cannot undef global register variable 'pinned'");
  });

  test('local explicit register collides', async () => {
    await requireThrowsWith(`
      u32<$t0> pinned;

      function test() {
        u32<$t0> x;
      }`, "Register '$t0' already used for variable 'pinned'");
  });

  test('command arg register collides', async () => {
    // implicit command args land in $a0.. - pinning one of those collides
    await requireThrowsWith(`
      u32<$a0> pinned;

      command<0> test(u32 arg) {
        arg += 1;
      }`, "Register '$a0' already used for variable 'pinned'");
  });

  test('built-in register collides', async () => {
    await requireThrowsWith(`
      vec16<$v30> pinned;

      function test() {}`, "Register '$v30' already used for variable 'VSHIFT'");
  });

  test('duplicate declaration', async () => {
    await requireThrowsWith(`
      u32<$t0> pinned;
      u32<$t1> pinned;

      function test() {}`, "Global variable 'pinned' already declared");
  });

  test('const global rejects writes', async () => {
    await requireThrowsWith(`
      const u32<$t0> pinned;

      function test() {
        pinned += 1;
      }`, "const");
  });

  test('parse errors', async () => {
    // register is mandatory
    await requireThrowsWith(`
      u32 pinned;

      function test() {}`, "must specify a register");
    // one name per declaration
    await requireThrowsWith(`
      u32<$t0> a, b;

      function test() {}`, "one per statement");
    // no initializers at file scope
    await requireThrowsWith(`
      u32<$t0> a = 1;

      function test() {}`, "cannot have an initializer");
  });

  test('interleaved with state section', async () => {
    const {asm, warn} = await transpileSource(`
      u32<$k1> globA;

      state { u32 SOME_VALUE; }

      vec16<$v20> globB;

      function test() {
        globA += 1;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toContain("addiu $k1, $k1, 1");
  });
});

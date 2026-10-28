import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

async function body(src) {
  const {asm, warn} = await transpileSource(`function test() { ${src} }`, CONF);
  expect(warn).toBe("");
  return asm.split("\n").slice(1, -2).map(l => l.trim());
}

describe('Compare (scalar into variable)', () =>
{
  test('unsigned variables use sltu', async () => {
    expect(await body(`u32<$t0> a; u32<$t1> b; u8<$t2> x = a < b;`)).toEqual(["sltu $t2, $t0, $t1"]);
    expect(await body(`u32<$t0> a; u32<$t1> b; u8<$t2> x = a > b;`)).toEqual(["sltu $t2, $t1, $t0"]);
  });

  test('signed variables use slt', async () => {
    expect(await body(`s32<$t0> a; s32<$t1> b; u8<$t2> x = a < b;`)).toEqual(["slt $t2, $t0, $t1"]);
    expect(await body(`s16<$t0> a; s16<$t1> b; u8<$t2> x = a > b;`)).toEqual(["slt $t2, $t1, $t0"]);
  });

  test('constant on the right uses the immediate form', async () => {
    expect(await body(`u32<$t0> a; u8<$t2> x = a < 5;`)).toEqual(["sltiu $t2, $t0, 5"]);
    expect(await body(`s32<$t0> a; u8<$t2> x = a < -3;`)).toEqual(["slti $t2, $t0, -3"]);
  });

  test('constant that does not fit is loaded first', async () => {
    const lines = await body(`u32<$t0> a; u8<$t2> x = a < 0x12345;`);
    expect(lines[lines.length - 1]).toBe("sltu $t2, $t0, $at");
    expect(lines.length).toBeGreaterThan(1);
    expect(lines.slice(0, -1).every(l => l.includes("$at"))).toBe(true);
  });

  test('greater than a constant loads it, zero uses $zero', async () => {
    const lines = await body(`u32<$t0> a; u8<$t2> x = a > 5;`);
    expect(lines[lines.length - 1]).toBe("sltu $t2, $at, $t0");
    expect(await body(`s32<$t0> a; u8<$t2> x = a > 0;`)).toEqual(["slt $t2, $zero, $t0"]);
  });
});

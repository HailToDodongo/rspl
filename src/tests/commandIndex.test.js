import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: true, optimize: false};

function commandTable(asm) {
  return asm.split("\n").filter(l => l.includes("RSPQ_DefineCommand")).map(l => l.trim());
}

describe('Command index', () =>
{
  test('omitted indices count up from 0', async () => {
    const {asm, warn} = await transpileSource(`
      state { u32 FOO; }
      command Cmd_A(u32 a) { store(a, FOO); }
      command Cmd_B(u32 a) { store(a, FOO); }
      command Cmd_C(u32 a) { store(a, FOO); }
    `, CONF);
    expect(warn).toBe("");
    expect(commandTable(asm)).toEqual([
      "RSPQ_DefineCommand Cmd_A, 4",
      "RSPQ_DefineCommand Cmd_B, 4",
      "RSPQ_DefineCommand Cmd_C, 4",
    ]);
  });

  test('omitted indices continue after an explicit one', async () => {
    const {asm} = await transpileSource(`
      state { u32 FOO; }
      command<2> Cmd_A(u32 a) { store(a, FOO); }
      command Cmd_B(u32 a) { store(a, FOO); }
      command<0> Cmd_C(u32 a) { store(a, FOO); }
      command Cmd_D(u32 a) { store(a, FOO); }
    `, CONF);
    expect(commandTable(asm)).toEqual([
      "RSPQ_DefineCommand Cmd_C, 4", // 0
      "RSPQ_DefineCommand Cmd_D, 4", // 1
      "RSPQ_DefineCommand Cmd_A, 4", // 2
      "RSPQ_DefineCommand Cmd_B, 4", // 3
    ]);
  });

  test('two commands on the same index are an error', async () => {
    await expect(transpileSource(`
      state { u32 FOO; }
      command Cmd_A(u32 a) { store(a, FOO); }
      command<0> Cmd_B(u32 a) { store(a, FOO); }
    `, CONF)).rejects.toThrow(/index 0 is already used by 'Cmd_A'/);
  });
});

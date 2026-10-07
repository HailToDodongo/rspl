import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Command Args', () =>
{
  test('args past $a0-$a3 are loaded from the buffer', async () => {
    const {asm, warn} = await transpileSource(`command<0> Cmd_Test(u32 a, u32 b, u32 c, u32 d, u32<$s5> e, u16<$s6> f)
{
  a += e;
  b += f;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`Cmd_Test:
  lw $s5, %lo(RSPQ_DMEM_BUFFER  -8)($gp)
  lhu $s6, %lo(RSPQ_DMEM_BUFFER  -4)($gp)
  addu $a0, $a0, $s5
  addu $a1, $a1, $s6
  j RSPQ_Loop
  nop`);
  });

  test('typed loads and auto-allocated register', async () => {
    const {asm, warn} = await transpileSource(`command<1> Cmd_Auto(u32 a, u32 b, u32 c, u32 d, u32 e, u8<$t7> f)
{
  a += e;
  b += f;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`Cmd_Auto:
  lw $s7, %lo(RSPQ_DMEM_BUFFER  -8)($gp)
  lbu $t7, %lo(RSPQ_DMEM_BUFFER  -4)($gp)
  addu $a0, $a0, $s7
  addu $a1, $a1, $t7
  j RSPQ_Loop
  nop`);
  });

  test('exactly 4 args emit no loads', async () => {
    const {asm, warn} = await transpileSource(`command<2> Cmd_Four(u32 a, u32 b, u32 c, u32 d)
{
  a += d;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`Cmd_Four:
  addu $a0, $a0, $a3
  j RSPQ_Loop
  nop`);
  });

  test('functions never load args from the buffer', async () => {
    const {asm, warn} = await transpileSource(`function FuncManyArgs(u32 a, u32 b, u32 c, u32 d, u32<$s5> e)
{
  a += e;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`FuncManyArgs:
  addu $a0, $a0, $s5
  jr $ra
  nop`);
  });

  test('offsets scale with argument count', async () => {
    // 5 args -> argSize 20, arg 4 sits at 16 - 20 = -4
    const {asm, warn} = await transpileSource(`command<3> Cmd_Five(u32 a, u32 b, u32 c, u32 d, u32<$s5> e)
{
  a += e;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`Cmd_Five:
  lw $s5, %lo(RSPQ_DMEM_BUFFER  -4)($gp)
  addu $a0, $a0, $s5
  j RSPQ_Loop
  nop`);
  });
});

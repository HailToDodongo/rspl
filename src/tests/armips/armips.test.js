import {transpileSource} from "../../lib/transpiler";

const CONF = {armips: true, rspqWrapper: false};

describe('armips', () =>
{
  test('Accumulator reads select the part through the element', async () => {
    const {asm, warn} = await transpileSource(`
      function test() {
        vec16 a = get_acc_high();
        a = get_acc_mid();
        a = get_acc_low();
      }
    `, CONF);

    expect(warn).toBe("");
    expect(asm).toContain("vsar $v01, $v00, $v00[0]");
    expect(asm).toContain("vsar $v01, $v00, $v00[1]");
    expect(asm).toContain("vsar $v01, $v00, $v00[2]");
    expect(asm).not.toContain("COP2_ACC");
  });

  test('Command args past $a0-$a1 are loaded relative to $gp', async () => {
    const {asm, warn} = await transpileSource(`
      command Cmd_Test(u32 a, u32 b, u32 c, u8 d, u32<$s5> e)
      {
        a += c;
        b += d;
        a += e;
      }
    `, CONF);

    expect(warn).toBe("");
    expect(asm).toContain("lw $a2, 8($gp)");
    expect(asm).toContain("lbu $a3, 12($gp)");
    expect(asm).toContain("lw $s5, 16($gp)");
    expect(asm).toContain("addu $a0, $a0, $a2");
    expect(asm).toContain("addu $a1, $a1, $a3");
  });

  test('load_arg and get_cmd_address are relative to $gp', async () => {
    const {asm, warn} = await transpileSource(`
      command Cmd_Test(u32 a, u32 b, u32 c)
      {
        u16<$t0> x = load_arg(6);
        u32<$t1> y = get_cmd_address(8);
      }
    `, CONF);

    expect(warn).toBe("");
    expect(asm).toContain("lhu $t0, 6($gp)");
    expect(asm).toContain("addiu $t1, $gp, 8");
  });
});

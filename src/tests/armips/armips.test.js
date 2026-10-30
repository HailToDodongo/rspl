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
});

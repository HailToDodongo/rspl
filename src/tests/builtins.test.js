import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Builtins', () =>
{
  test('swap() - scalar', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        u32 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  xor $t0, $t0, $t1
  xor $t1, $t0, $t1
  xor $t0, $t0, $t1
  jr $ra
  nop`);
  });

  test('swap() - vec16', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec16 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v01, $v01, $v02
  vxor $v02, $v01, $v02
  vxor $v01, $v01, $v02
  jr $ra
  nop`);
  });

  test('swap() - vec32', async () => {
    const {asm, warn} = await transpileSource(`function test() {
        vec32 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v01, $v01, $v03
  vxor $v03, $v01, $v03
  vxor $v01, $v01, $v03
  vxor $v02, $v02, $v04
  vxor $v04, $v02, $v04
  vxor $v02, $v02, $v04
  jr $ra
  nop`);
  });
});
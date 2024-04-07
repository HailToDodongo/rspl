import {transpileSource} from "../../../lib/transpiler";

const CONF = {rspqWrapper: false, optimize: true};

describe('Optimizer E2E - Merge Sequence', () =>
{
  test('Multiply - Zero fractional', async () => {
    const {asm, warn} = await transpileSource(`
    state { vec16 SCREEN_SCALE_OFFSET; }
    function test(u32 dummy) 
    {
      vec32 screenSize;
      screenSize:sint = load(SCREEN_SCALE_OFFSET);
      screenSize:sfract = 0;
      screenSize >>= 8;
      END:
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)
  lqv $v01, 0, 0, $at
  vmudl $v02, $v00, $v31.e7
  vmadm $v01, $v01, $v31.e7
  vmadn $v02, $v00, $v00
  END:
  jr $ra
  nop`);
  });

  test('Multiply - Non-Zero fractional (no opt)', async () => {
    const {asm, warn} = await transpileSource(`
    state { vec16 SCREEN_SCALE_OFFSET; }
    function test(u32 dummy) 
    {
      vec32 screenSize;
      screenSize:sint = load(SCREEN_SCALE_OFFSET);
      screenSize:sfract = 1;
      screenSize >>= 8;
      END:
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(SCREEN_SCALE_OFFSET)
  lqv $v01, 0, 0, $at
  vxor $v02, $v00, $v30.e7
  vmudl $v02, $v02, $v31.e7
  vmadm $v01, $v01, $v31.e7
  vmadn $v02, $v00, $v00
  END:
  jr $ra
  nop`);
  });

});
import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Annotations', () =>
{
  test('Align (function)', async () => {
    const {asm, warn} = await transpileSource(`
    @Align(8)
    function test() 
    {
      exit;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(
`.align 3
test:
  j RSPQ_Loop
  nop
  jr $ra
  nop`);
  });

  test('Relative (function)', async () => {
    const {asm, warn} = await transpileSource(`
    @Relative
    function target_rel() {}
    function target_abs() {}
    function caller() {
      target_rel();
      target_abs();
    }
    `, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(
`target_rel:
  jr $ra
  nop
target_abs:
  jr $ra
  nop
caller:
  bgezal $zero, target_rel
  nop
  jal target_abs
  nop
  jr $ra
  nop`);
  });

  test('Relative (caller)', async () => {
    const {asm, warn} = await transpileSource(`
    function target_rel() {}
    function target_abs() {}
    function caller() {
      @Relative target_rel();
      target_abs();
    }
    `, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(
`target_rel:
  jr $ra
  nop
target_abs:
  jr $ra
  nop
caller:
  bgezal $zero, target_rel
  nop
  jal target_abs
  nop
  jr $ra
  nop`);
  });
});
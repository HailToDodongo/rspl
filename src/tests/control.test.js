import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Control', () =>
{
  test('Exit', async () => {
    const {asm, warn} = await transpileSource(`function test() 
{
  exit;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  j RSPQ_Loop
  nop
  jr $ra
  nop`);
  });
});
import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Control', () =>
{
  test('Exit', () => {
    const {asm, warn} = transpileSource(`function test() 
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
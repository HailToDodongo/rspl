import {transpileSource} from "../../../lib/transpiler";

const CONF = {rspqWrapper: false, optimize: true};

describe('Optimizer E2E - Jump Dedupe', () =>
{
  test('Nested-If - Used Label', async () => {
    const {asm, warn} = await transpileSource(`command<0> test() 
    {
      u32 a = 1;
      if(a > 1) {
        if(a > 10) {
          a += 1;
        }
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $s7, $zero, 1
  sltiu $at, $s7, 2
  bne $at, $zero, RSPQ_Loop
  nop
  sltiu $at, $s7, 11
  bne $at, $zero, RSPQ_Loop
  nop
  addiu $s7, $s7, 1
  LABEL_0001:
  j RSPQ_Loop
  nop`);
  });

  test('Nested-If - Unused Label', async () => {
    const {asm, warn} = await transpileSource(`command<0> test() 
    {
      u32 a = 1;
      while(a < 2) {
        a -= 1;
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  addiu $s7, $zero, 1
  LABEL_0001:
  sltiu $at, $s7, 2
  beq $at, $zero, RSPQ_Loop
  nop
  j LABEL_0001
  addiu $s7, $s7, 65535`);
  });
});
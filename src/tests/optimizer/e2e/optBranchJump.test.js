import {transpileSource} from "../../../lib/transpiler";

const CONF = {rspqWrapper: false, optimize: true};

describe('Optimizer E2E - Branch-Jump', () =>
{
  test('Branch + Goto', () => {
    const {asm, warn} = transpileSource(`function test() 
    {
      u32<$t0> a;
      LABEL_A:
      if(a != 0)goto LABEL_A;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  LABEL_A:
  bne $t0, $zero, LABEL_A
  nop
  jr $ra
  nop`);
  });

  test('Branch + Goto (no opt)', () => {
    const {asm, warn} = transpileSource(`function test() 
    {
      u32<$t0> a;
      LABEL_A:
      if(a != 0) {
        a += 1;
        goto LABEL_A;
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  LABEL_A:
  beq $t0, $zero, LABEL_0001
  nop
  j LABEL_A
  addiu $t0, $t0, 1
  LABEL_0001:
  jr $ra
  nop`);
  });
});
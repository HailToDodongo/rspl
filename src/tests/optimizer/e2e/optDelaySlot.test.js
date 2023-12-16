import {transpileSource} from "../../../lib/transpiler";

const CONF = {rspqWrapper: false, optimize: true};

describe('Optimizer E2E - Delay-Slots', () =>
{
  test('Fill - Basic', () => {
    const {asm, warn} = transpileSource(`function test(u32 dummy) 
    {
      u32 a = 1;
      goto SOME_LABEL;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  j SOME_LABEL
  addiu $t0, $zero, 1
  jr $ra
  nop`);
  });

  test('Fill - Complex', () => {
    const {asm, warn} = transpileSource(`function test(u32 i) 
    {
      u32 test = 0;
      while(i != 0) {
        if(i == 6) {
          test = 42;
          break;
        }         
        i -= 1; 
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero
  LABEL_0001:
  beq $a0, $zero, LABEL_0002
  nop
  addiu $at, $zero, 6
  bne $a0, $at, LABEL_0003
  nop
  j LABEL_0002
  addiu $t0, $zero, 42
  LABEL_0003:
  j LABEL_0001
  addiu $a0, $a0, 65535
  LABEL_0002:
  jr $ra
  nop`);
  });
});
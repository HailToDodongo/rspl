import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Loops', () =>
{
  test('Basic While-Loop', () => {
    const {asm, warn} = transpileSource(`function test()
    {
      u32<$t0> i=0;
      while(i<10) {
        i+=1;
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero
  1:
  sltiu $at, $t0, 10
  beq $at, $zero, 2f
  nop
  addiu $t0, $t0, 1
  j 1b
  nop
  2:
  jr $ra
  nop`);
  });

  test('Nested While-Loop', () => {
    const {asm, warn} = transpileSource(`function test()
    {
      u32<$t0> i=0;
      u32<$t1> j=0;
      
      while(i<10) {
        while(j<20) {
          j+=1;
        }
        i+=1;
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero
  or $t1, $zero, $zero
  1:
  sltiu $at, $t0, 10
  beq $at, $zero, 2f
  nop
  3:
  sltiu $at, $t1, 20
  beq $at, $zero, 4f
  nop
  addiu $t1, $t1, 1
  j 3b
  nop
  4:
  addiu $t0, $t0, 1
  j 1b
  nop
  2:
  jr $ra
  nop`);
  });

  test('While Loop - Break', () => {
    const {asm, warn} = transpileSource(`function test()
    {
      u32<$t0> i=0;
      while(i<10) {
        break;
        i+=1;
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero
  1:
  sltiu $at, $t0, 10
  beq $at, $zero, 2f
  nop
  j 2f
  nop
  addiu $t0, $t0, 1
  j 1b
  nop
  2:
  jr $ra
  nop`);
  });

  test('While Loop - scoped Break', () => {
    const {asm, warn} = transpileSource(`function test()
    {
      u32<$t0> i=0;
      while(i<10) {
        if(!i)break;
        i+=1;
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero
  1:
  sltiu $at, $t0, 10
  beq $at, $zero, 2f
  nop
  bne $t0, $zero, 3f
  nop
  j 2f
  nop
  3:
  addiu $t0, $t0, 1
  j 1b
  nop
  2:
  jr $ra
  nop`);
  });

  test('While Loop - Continue', () => {
    const {asm, warn} = transpileSource(`function test()
    {
      u32<$t0> i=0;
      while(i<10) {
        continue;
        i+=1;
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero
  1:
  sltiu $at, $t0, 10
  beq $at, $zero, 2f
  nop
  j 1b
  nop
  addiu $t0, $t0, 1
  j 1b
  nop
  2:
  jr $ra
  nop`);
  });
});
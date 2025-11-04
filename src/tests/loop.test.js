import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Loops', () =>
{
  test('Basic While-Loop', async () => {
    const {asm, warn} = await transpileSource(`function test()
    {
      u32<$t0> i=0;
      while(i<10) {
        i+=1;
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero
  LABEL_test_0001:
  sltiu $at, $t0, 10
  beq $at, $zero, LABEL_test_0002
  nop
  addiu $t0, $t0, 1
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop`);
  });

  test('Nested While-Loop', async () => {
    const {asm, warn} = await transpileSource(`function test()
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
  LABEL_test_0001:
  sltiu $at, $t0, 10
  beq $at, $zero, LABEL_test_0002
  nop
  LABEL_test_0003:
  sltiu $at, $t1, 20
  beq $at, $zero, LABEL_test_0004
  nop
  addiu $t1, $t1, 1
  j LABEL_test_0003
  nop
  LABEL_test_0004:
  addiu $t0, $t0, 1
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop`);
  });

  test('While Loop - Break', async () => {
    const {asm, warn} = await transpileSource(`function test()
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
  LABEL_test_0001:
  sltiu $at, $t0, 10
  beq $at, $zero, LABEL_test_0002
  nop
  j LABEL_test_0002
  nop
  addiu $t0, $t0, 1
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop`);
  });

  test('While Loop - scoped Break', async () => {
    const {asm, warn} = await transpileSource(`function test()
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
  LABEL_test_0001:
  sltiu $at, $t0, 10
  beq $at, $zero, LABEL_test_0002
  nop
  bne $t0, $zero, LABEL_test_0003
  nop
  j LABEL_test_0002
  nop
  LABEL_test_0003:
  addiu $t0, $t0, 1
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop`);
  });

  test('While Loop - Continue', async () => {
    const {asm, warn} = await transpileSource(`function test()
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
  LABEL_test_0001:
  sltiu $at, $t0, 10
  beq $at, $zero, LABEL_test_0002
  nop
  j LABEL_test_0001
  nop
  addiu $t0, $t0, 1
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop`);
  });
});
import {transpileSource} from "../../../lib/transpiler";

const CONF = {rspqWrapper: false, optimize: true};

describe('Optimizer E2E - Branch-Jump', () =>
{
  test('Branch + Goto', async () => {
    const {asm, warn} = await transpileSource(`function test() 
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

  test('Branch + Goto (no opt)', async () => {
    const {asm, warn} = await transpileSource(`function test() 
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
  beq $t0, $zero, LABEL_test_0001
  nop
  j LABEL_A
  addiu $t0, $t0, 1
  LABEL_test_0001:
  jr $ra
  nop`);
  });

  test('Loop - Used Label', async () => {
    // 'SOME_LABEL' here happens to be after a branch that will be optimized
    // however we can't remove it since something later on will reference it
    const {asm, warn} = await transpileSource(`function test() 
    {
      u32<$t0> a;
      loop {        
        if(a == 1)continue;
        SOME_LABEL:
        
        if(a == 0)goto SOME_LABEL;
        LOOP_END:
      }
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  LABEL_test_0001:
  addiu $at, $zero, 1
  beq $t0, $at, LABEL_test_0001
  nop
  SOME_LABEL:
  bne $t0, $zero, LABEL_test_0001
  nop
  j SOME_LABEL
  nop
  LOOP_END:
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop`);
  });

  test('Loop - Unused Label', async () => {
    // Same as a above, but this time the label is actually unused
    const {asm, warn} = await transpileSource(`function test() 
    {
      u32<$t0> a;
      loop {        
        if(a == 1)continue;
        SOME_LABEL:
        
        if(a == 0)continue;
        LOOP_END:
      }
    }`, CONF);

    expect(warn).toBe("");

    expect(asm).toBe(`test:
  LABEL_test_0001:
  addiu $at, $zero, 1
  beq $t0, $at, LABEL_test_0001
  nop
  SOME_LABEL:
  bne $t0, $zero, LABEL_test_0001
  nop
  j LABEL_test_0001
  nop
  LOOP_END:
  j LABEL_test_0001
  nop
  LABEL_test_0002:
  jr $ra
  nop`);
  });

  test('keep user label after if-goto', async () => {
    const {asm, warn} = await transpileSource(`state { extern u16 RSPQ_Loop; }
    function other()
    {
      goto L_B;
    }
    function test()
    {
      u32<$t0> a;
      u32<$t1> b;
      L_A:
        a += 1;
        if(a == b) goto RSPQ_Loop;
      L_B:
      {
        a += 2;
      }
      goto L_A;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`other:
  j L_B
  nop
test:
  L_A:
  addiu $t0, $t0, 1
  beq $t0, $t1, RSPQ_Loop
  nop
  L_B:
  j L_A
  addiu $t0, $t0, 2`);
  });
});
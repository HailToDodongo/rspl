import {transpileSource} from "../../../lib/transpiler";

describe('Optimizer E2E - Assertion', () =>
{
  test('Assert variations (unopt)', async () => {
    const {asm, warn} = await transpileSource(`function a() 
    {
     u32 buff,test;
     TEST_A:
     if(buff > 4)assert(0xAB);
     
     TEST_B:
     if(buff < 4)assert(0xAB);
     
     TEST_C:
     if(buff == 4)assert(0xAB);
     
     TEST_D:
     if(buff != 4)assert(0xAB);
     
     TEST_E:
     if(buff == 0)assert(0xAB);
     
     TEST_F:
     if(buff != 0)assert(0xAB);
     
     TEST_G:
     if(buff != test)assert(0xAB);
     
     TEST_H:
     if(buff < test)assert(0xAB);
     
     TEST_I:
     assert(0xAB);
    }`, {rspqWrapper: false, optimize: false});

    expect(warn).toBe("");
    expect(asm).toBe(`a:
  TEST_A:
  sltiu $at, $t0, 5
  bne $at, $zero, LABEL_a_0001
  nop
  lui $at, 171
  j assertion_failed
  nop
  LABEL_a_0001:
  TEST_B:
  sltiu $at, $t0, 4
  beq $at, $zero, LABEL_a_0002
  nop
  lui $at, 171
  j assertion_failed
  nop
  LABEL_a_0002:
  TEST_C:
  addiu $at, $zero, 4
  bne $t0, $at, LABEL_a_0003
  nop
  lui $at, 171
  j assertion_failed
  nop
  LABEL_a_0003:
  TEST_D:
  addiu $at, $zero, 4
  beq $t0, $at, LABEL_a_0004
  nop
  lui $at, 171
  j assertion_failed
  nop
  LABEL_a_0004:
  TEST_E:
  bne $t0, $zero, LABEL_a_0005
  nop
  lui $at, 171
  j assertion_failed
  nop
  LABEL_a_0005:
  TEST_F:
  beq $t0, $zero, LABEL_a_0006
  nop
  lui $at, 171
  j assertion_failed
  nop
  LABEL_a_0006:
  TEST_G:
  beq $t0, $t1, LABEL_a_0007
  nop
  lui $at, 171
  j assertion_failed
  nop
  LABEL_a_0007:
  TEST_H:
  sltu $at, $t0, $t1
  beq $at, $zero, LABEL_a_0008
  nop
  lui $at, 171
  j assertion_failed
  nop
  LABEL_a_0008:
  TEST_I:
  lui $at, 171
  j assertion_failed
  nop
  jr $ra
  nop`);
  });

  test('Assert variations (opt)', async () => {
    const {asm, warn} = await transpileSource(`function a() 
    {
     u32 buff,test;
     TEST_A:
     if(buff > 4)assert(0xAB);
     
     TEST_B:
     if(buff < 4)assert(0xAB);
     
     TEST_C:
     if(buff == 4)assert(0xAB);
     
     TEST_D:
     if(buff != 4)assert(0xAB);
     
     TEST_E:
     if(buff == 0)assert(0xAB);
     
     TEST_F:
     if(buff != 0)assert(0xAB);
     
     TEST_G:
     if(buff != test)assert(0xAB);
     
     TEST_H:
     if(buff < test)assert(0xAB);
     
     TEST_I:
     assert(0xAB);
    }`, {rspqWrapper: false, optimize: true});

    expect(warn).toBe("");
    expect(asm).toBe(`a:
  TEST_A:
  sltiu $at, $t0, 5
  beq $at, $zero, assertion_failed
  lui $at, 171
  TEST_B:
  sltiu $at, $t0, 4
  bne $at, $zero, assertion_failed
  lui $at, 171
  TEST_C:
  addiu $at, $zero, 4
  beq $t0, $at, assertion_failed
  lui $at, 171
  TEST_D:
  addiu $at, $zero, 4
  bne $t0, $at, assertion_failed
  lui $at, 171
  TEST_E:
  beq $t0, $zero, assertion_failed
  lui $at, 171
  TEST_F:
  bne $t0, $zero, assertion_failed
  lui $at, 171
  TEST_G:
  bne $t0, $t1, assertion_failed
  lui $at, 171
  TEST_H:
  sltu $at, $t0, $t1
  bne $at, $zero, assertion_failed
  lui $at, 171
  TEST_I:
  j assertion_failed
  lui $at, 171`);
  });
});
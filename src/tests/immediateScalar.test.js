import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Immediate (Scalar)', () =>
{
  test('Unsigned', () =>
  {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
    function test() 
    {
      u32<$t0> c = TEST_CONST;
      LOAD_A:  c = 0;          
      LOAD_B:  c = 0xFF;       
      LOAD_C:  c = 0xFFFF;      
      LOAD_D:  c = 0x7FFF;     
      LOAD_E:  c = 0x8000;     
      LOAD_F:  c = 0xFF120000; 
      LOAD_G:  c = 0xFFFF7FFF; 
      LOAD_H:  c = 0xFFFF8000; 
      LOAD_I:  c = 0xFFFFF;    
      LOAD_J:  c = 0xFFFFFFFF; 
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $t0, $zero, %lo(TEST_CONST)
  LOAD_A:
  or $t0, $zero, $zero
  LOAD_B:
  addiu $t0, $zero, 255
  LOAD_C:
  ori $t0, $zero, 0xFFFF
  LOAD_D:
  addiu $t0, $zero, 32767
  LOAD_E:
  ori $t0, $zero, 0x8000
  LOAD_F:
  lui $t0, 0xFF12
  LOAD_G:
  lui $t0, 0xFFFF
  ori $t0, $t0, 0x7FFF
  LOAD_H:
  addiu $t0, $zero, -32768
  LOAD_I:
  lui $t0, 0x0F
  ori $t0, $t0, 0xFFFF
  LOAD_J:
  addiu $t0, $zero, -1
  jr $ra
  nop`);
  });

  test('Signed', () =>
  {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
    function test() 
    {
      s32<$t0> c = TEST_CONST;
      LOAD_A:  c = 0xFF;
      LOAD_B:  c = 0xFFFF;
      LOAD_C:  c = 0xFFFFF;
      LOAD_D:  c = -255;
      LOAD_E:  c = -65535;
      LOAD_F:  c = -1048575;
      LOAD_G:  c = -2147483648;
      LOAD_H:  c = 2147483647;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $t0, $zero, %lo(TEST_CONST)
  LOAD_A:
  addiu $t0, $zero, 255
  LOAD_B:
  ori $t0, $zero, 0xFFFF
  LOAD_C:
  lui $t0, 0x0F
  ori $t0, $t0, 0xFFFF
  LOAD_D:
  addiu $t0, $zero, -255
  LOAD_E:
  lui $t0, 0xFFFF
  ori $t0, $t0, 0x01
  LOAD_F:
  lui $t0, 0xFFF0
  ori $t0, $t0, 0x01
  LOAD_G:
  lui $t0, 0x8000
  LOAD_H:
  lui $t0, 0x7FFF
  ori $t0, $t0, 0xFFFF
  jr $ra
  nop`);
  });

});
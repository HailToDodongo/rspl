import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Immediate (Scalar)', () =>
{
  test('Unsigned', () =>
  {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
    function test() 
    {
      u32<$t0> c = TEST_CONST; //
      c = 0;          //
      c = 0xFF;       //
      c = 0xFFFF;     // 
      c = 0x7FFF;     //
      c = 0x8000;     //
      c = 0xFF120000; //
      c = 0xFFFF7FFF; //
      c = 0xFFFF8000; //
      c = 0xFFFFF;    //
      c = 0xFFFFFFFF; //
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $t0, $zero, %lo(TEST_CONST)
  ##
  or $t0, $zero, $zero
  ##
  addiu $t0, $zero, 255
  ##
  ori $t0, $zero, 0xFFFF
  ##
  addiu $t0, $zero, 32767
  ##
  ori $t0, $zero, 0x8000
  ##
  lui $t0, 0xFF12
  ##
  lui $t0, 0xFFFF
  ori $t0, $t0, 0x7FFF
  ##
  addiu $t0, $zero, -32768
  ##
  lui $t0, 0x0F
  ori $t0, $t0, 0xFFFF
  ##
  addiu $t0, $zero, -1
  ##
  jr $ra
  nop`);
  });

  test('Signed', () =>
  {
    const {asm, warn} = transpileSource(` state { u32 TEST_CONST; }
    function test() 
    {
      s32<$t0> c = TEST_CONST; //
      c = 0xFF;                //
      c = 0xFFFF;              //
      c = 0xFFFFF;             //
      c = -255;                //
      c = -65535;              //
      c = -1048575;            //
      c = -2147483648;         //
      c = 2147483647;          //
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $t0, $zero, %lo(TEST_CONST)
  ##
  addiu $t0, $zero, 255
  ##
  ori $t0, $zero, 0xFFFF
  ##
  lui $t0, 0x0F
  ori $t0, $t0, 0xFFFF
  ##
  addiu $t0, $zero, -255
  ##
  lui $t0, 0xFFFF
  ori $t0, $t0, 0x01
  ##
  lui $t0, 0xFFF0
  ori $t0, $t0, 0x01
  ##
  lui $t0, 0x8000
  ##
  lui $t0, 0x7FFF
  ori $t0, $t0, 0xFFFF
  ##
  jr $ra
  nop`);
  });

});
import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Macros', () =>
{
  test('Basic replacement', () => {
    const {asm, warn} = transpileSource(`
      macro test(u32 add) {
        add += 42;
      }
      
      function test_macro() {
        u32<$t2> a;
        u32<$s3> b;
        test(a);
        
        if(a < 3) {
          test(a);
        }
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_macro:
  addiu $t2, $t2, 42
  sltiu $at, $t2, 3
  beq $at, $zero, 1f
  nop
  addiu $t2, $t2, 42
  1:
  jr $ra
  nop`);
  });
});
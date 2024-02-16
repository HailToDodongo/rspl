import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Macros', () =>
{
  test('Basic replacement', async () => {
    const {asm, warn} = await transpileSource(`
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
  beq $at, $zero, LABEL_0001
  nop
  addiu $t2, $t2, 42
  LABEL_0001:
  jr $ra
  nop`);
  });

  test('Nested macro', async () => {
    const {asm, warn} = await transpileSource(`
      macro test_b(u32 argB) {
        argB += 42;
      }
      
      macro test_a(u32 argA) {
        test_b(argA);
      }
      
      function test_macro() {
        u32<$t2> a;
        test_a(a);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_macro:
  addiu $t2, $t2, 42
  jr $ra
  nop`);
  });

  test('Scope local', async () => {
    const {asm, warn} = await transpileSource(`
      macro test_b(u32 argB) {
        argB += 42;
      }
      
      function test_macro() {
        u32<$t2> a;
        u32<$t3> argB;
        test_b(a);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test_macro:
  addiu $t2, $t2, 42
  jr $ra
  nop`);
  });
});
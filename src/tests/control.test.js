import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Control', () =>
{
  test('Exit', async () => {
    const {asm, warn} = await transpileSource(`function test() 
{
  exit;
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  j RSPQ_Loop
  nop
  jr $ra
  nop`);
  });

  test('Unlikely If', async () => {
    const {asm, warn} = await transpileSource(`state { u32 FOO; }
function test()
{
  u32<$t0> a = load(FOO);
  @Unlikely if(a != 0)
  {
    a += 1;
    store(a, FOO);
  }
  a += 2;
  store(a, FOO);
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  lw $t0, %lo(FOO + 0)
  bne $t0, $zero, LABEL_test_0001
  nop
  LABEL_test_0002:
  addiu $t0, $t0, 2
  sw $t0, %lo(FOO)($zero)
  jr $ra
  nop
  LABEL_test_0001:
  addiu $t0, $t0, 1
  sw $t0, %lo(FOO)($zero)
  j LABEL_test_0002
  nop`);
  });

  test('Unlikely If (slt compare)', async () => {
    const {asm, warn} = await transpileSource(`state { u32 FOO; }
function test()
{
  u32<$t0> a = load(FOO);
  u32<$t1> b = load(FOO);
  @Unlikely if(a < b)
  {
    a += 1;
  }
  store(a, FOO);
}`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  lw $t0, %lo(FOO + 0)
  lw $t1, %lo(FOO + 0)
  sltu $at, $t0, $t1
  bne $at, $zero, LABEL_test_0001
  nop
  LABEL_test_0002:
  sw $t0, %lo(FOO)($zero)
  jr $ra
  nop
  LABEL_test_0001:
  addiu $t0, $t0, 1
  j LABEL_test_0002
  nop`);
  });

  test('Unlikely If - else not allowed', async () => {
    const src = `state { u32 FOO; }
function test()
{
  u32<$t0> a = load(FOO);
  @Unlikely if(a != 0) {
    a += 1;
  } else {
    a += 2;
  }
  store(a, FOO);
}`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/else-block/);
  });
});

import {transpileSource} from "../../../lib/transpiler";

const CONF = {rspqWrapper: false, optimize: true};

describe('Optimizer E2E - Deduplicate Immediate', () =>
{
  test('$at cached across consecutive loads from same state var', async () => {
    const {asm, warn} = await transpileSource(`
state {
  vec16 MY_VAR[4];
}
function test()
{
  vec16 a = load(MY_VAR, 0x00);
  vec16 b = load(MY_VAR, 0x10);
  vec16 c = load(MY_VAR, 0x20);
}
`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(MY_VAR)
  lqv $v02, 0, 16, $at
  lqv $v03, 0, 32, $at
  jr $ra
  lqv $v01, 0, 0, $at`);
  });

  test('$at NOT cached when branch intervenes', async () => {
    const {asm, warn} = await transpileSource(`
state {
  vec16 MY_VAR[4];
}
function test(u32 cond)
{
  vec16 a = load(MY_VAR, 0x00);
  if(cond != 0) {
    a += 1;
  }
  vec16 b = load(MY_VAR, 0x10);
}
`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(MY_VAR)
  beq $a0, $zero, LABEL_test_0001
  lqv $v01, 0, 0, $at
  vaddc $v01, $v01, $v30.e7
  LABEL_test_0001:
  ori $at, $zero, %lo(MY_VAR)
  jr $ra
  lqv $v02, 0, 16, $at`);
  });

  test('$at changes across different state vars', async () => {
    const {asm, warn} = await transpileSource(`
state {
  vec16 VAR_A[4];
  vec16 VAR_B[4];
}
function test()
{
  vec16 a = load(VAR_A, 0x00);
  vec16 b = load(VAR_B, 0x00);
}
`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(VAR_A)
  lqv $v01, 0, 0, $at
  ori $at, $zero, %lo(VAR_B)
  jr $ra
  lqv $v02, 0, 0, $at`);
  });

  test('$at recached after different var access', async () => {
    const {asm, warn} = await transpileSource(`
state {
  vec16 VAR_A[4];
  vec16 VAR_B[4];
}
function test()
{
  vec16 a = load(VAR_A, 0x00);
  vec16 b = load(VAR_B, 0x00);
  vec16 c = load(VAR_A, 0x10);
}
`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(VAR_A)
  lqv $v01, 0, 0, $at
  ori $at, $zero, %lo(VAR_B)
  lqv $v02, 0, 0, $at
  ori $at, $zero, %lo(VAR_A)
  jr $ra
  lqv $v03, 0, 16, $at`);
  });
});

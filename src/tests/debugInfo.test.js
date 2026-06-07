import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false, debugInfo: true};

describe('Debug Info', () => {

  test('Basic operations', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32 a = 0;
      a += 42;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero                               ## L:2    |      ^ | u32 a = 0;
  addiu $t0, $t0, 42                                 ## L:3    |      2 | a += 42;
  jr $ra                                             ## L:4    |      3 | }
  nop                                                ## L:4    |     *5 | }`);
  });

  test('@Tag annotation prefix on instruction', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32 a;
      @Tag("Foo") a = 1;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  TAG_Foo: addiu $t0, $zero, 1                                ## L:3    |      ^ | @Tag("Foo") a = 1;
  jr $ra                                             ## L:4    |      2 | }
  nop                                                ## L:4    |     *4 | }`);
  });

  test('@Tag annotation on label (used label keeps tag)', async () => {
    // Use goto so the label is referenced and not deduped
    const {asm, warn} = await transpileSource(`function test() {
      @Tag("Start") LOOP:
      u32 a = 1;
      goto LOOP;
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  TAG_Start: LOOP:
  addiu $t0, $zero, 1                                ## L:3    |      1 | u32 a = 1;
  j LOOP                                             ## L:4    |      2 | goto LOOP;
  nop                                                ## L:4    |     *4 | goto LOOP;`);
  });

  test('Transpose builtin shares line and barrier info', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v08> v0;
      u16 buff;
      v0 = transpose(v0, buff, 4, 4);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  stv $v08, 2, 16, $t0                               ## L:4    |      ^ | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  stv $v08, 4, 32, $t0                               ## L:4    |      2 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  stv $v08, 6, 48, $t0                               ## L:4    |      3 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  stv $v08, 10, 80, $t0                              ## L:4    |      4 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  stv $v08, 12, 96, $t0                              ## L:4    |      5 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  stv $v08, 14, 112, $t0                             ## L:4    |      6 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  ltv $v08, 14, 16, $t0                              ## L:4    |      7 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  ltv $v08, 12, 32, $t0                              ## L:4    |      8 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  ltv $v08, 10, 48, $t0                              ## L:4    |      9 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  ltv $v08, 6, 80, $t0                               ## L:4    |     10 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  ltv $v08, 4, 96, $t0                               ## L:4    |     11 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  ltv $v08, 2, 112, $t0                              ## L:4    |     12 | v0 = transpose(v0, buff, 4, 4); ## Barrier: 0x1
  jr $ra                                             ## L:5    |     13 | }
  nop                                                ## L:5    |    *15 | }`);
  });

  test('Debug info disabled produces no padding or comments', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      u32 a = 0;
    }`, {rspqWrapper: false, debugInfo: false});

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  or $t0, $zero, $zero
  jr $ra
  nop`);
  });

});

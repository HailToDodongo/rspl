import {transpileSource} from "../../lib/transpiler";

const CONF = {magma: true};

describe('Magma mode', () =>
{
  test('No RSPQ-Header in magma mode', async () => {
    const {asm, warn} = await transpileSource(`
    shader testshader()
    {
    }`, CONF);

    expect(asm).not.toContain("RSPQ_BeginOverlayHeader");
    expect(asm).not.toContain("RSPQ_EndOverlayHeader");
  });

  test('No saved state in magma mode', async () => {
    const {asm, warn} = await transpileSource(`
    shader testshader()
    {
    }`, CONF);

    expect(asm).not.toContain("RSPQ_BeginSavedState");
    expect(asm).not.toContain("RSPQ_EndSavedState");
    expect(asm).not.toContain("RSPQ_EmptySavedState");
  });

  test('No sections in magma mode', async () => {
    const {asm, warn} = await transpileSource(`
    shader testshader()
    {
    }`, CONF);

    expect(asm).not.toContain(".data");
    expect(asm).not.toContain(".text");
  });

  test('Extern state', async () => {
    const {asm, warn} = await transpileSource(`
    state
    {
      extern u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    }`, CONF);

    expect(asm).toContain("lw $t0, %lo(VALUE + 0)");
    expect(asm).not.toContain("VALUE: .ds.b 4");
  });

  test('Extern data', async () => {
    const {asm, warn} = await transpileSource(`
    data
    {
      extern u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    }`, CONF);

    expect(asm).toContain("lw $t0, %lo(VALUE + 0)");
    expect(asm).not.toContain("VALUE: .ds.b 4");
  });

  test('Extern bss', async () => {
    const {asm, warn} = await transpileSource(`
    bss
    {
      extern u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    }`, CONF);

    expect(asm).toContain("lw $t0, %lo(VALUE + 0)");
    expect(asm).not.toContain("VALUE: .ds.b 4");
  });

  test('Non-extern state in magma mode', async () => {
    const src = `
    state
    {
      u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Only extern states are allowed when compiling for magma!/);
  });

  test('Non-extern data in magma mode', async () => {
    const src = `
    data
    {
      u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Only extern states are allowed when compiling for magma!/);
  });

  test('Non-extern bss in magma mode', async () => {
    const src = `
    bss
    {
      u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Only extern states are allowed when compiling for magma!/);
  });
});
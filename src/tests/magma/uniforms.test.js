import {transpileSource} from "../../lib/transpiler";

const CONF = {magma: true};

const getUniforms = asm => {
  const idxBegin = asm.indexOf("MgBeginShaderUniforms");
  const endKeyword = "MgEndShaderUniforms";
  const idxEnd = asm.indexOf(endKeyword);
  return asm.substring(idxBegin, idxEnd + endKeyword.length);
}

describe('Uniforms', () =>
{
  test('Single uniform', async () => {
    const {asm, warn} = await transpileSource(`
    uniform<0> UNIFORM0
    {
        u32 VALUE0;
    }
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getUniforms(asm)).toBe(
`MgBeginShaderUniforms
  MgBeginUniform UNIFORM0, 0
    .align 2
    VALUE0: .ds.b 4
  MgEndUniform

MgEndShaderUniforms`);
  });

  test('Multiple values', async () => {
    const {asm, warn} = await transpileSource(`
    uniform<0> UNIFORM0
    {
        vec16 POSITIONS[2];
        u32 VALUE0;
        u32 VALUES[4];
    }
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getUniforms(asm)).toBe(
`MgBeginShaderUniforms
  MgBeginUniform UNIFORM0, 0
    .align 4
    POSITIONS: .ds.b 32
    .align 2
    VALUE0: .ds.b 4
    .align 2
    VALUES: .ds.b 16
  MgEndUniform

MgEndShaderUniforms`);
  });

  test('Multiple uniforms', async () => {
    const {asm, warn} = await transpileSource(`
    uniform<0> UNIFORM0
    {
        u32 VALUE0;
    }

    uniform<1> UNIFORM1
    {
        s16 POSITION[3];
    }
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getUniforms(asm)).toBe(
`MgBeginShaderUniforms
  MgBeginUniform UNIFORM0, 0
    .align 2
    VALUE0: .ds.b 4
  MgEndUniform

  MgBeginUniform UNIFORM1, 1
    .align 1
    POSITION: .ds.b 6
  MgEndUniform

MgEndShaderUniforms`);
  });

  test('Arbitrary binding numbers', async () => {
    const {asm, warn} = await transpileSource(`
    uniform<748> UNIFORM0
    {
        u32 VALUE0;
    }
    
    uniform<34> UNIFORM1
    {
        u32 VALUE1;
    }
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getUniforms(asm)).toBe(
`MgBeginShaderUniforms
  MgBeginUniform UNIFORM0, 748
    .align 2
    VALUE0: .ds.b 4
  MgEndUniform

  MgBeginUniform UNIFORM1, 34
    .align 2
    VALUE1: .ds.b 4
  MgEndUniform

MgEndShaderUniforms`);
  });

  test('Omitted binding numbers', async () => {
    const {asm, warn} = await transpileSource(`
    uniform<1> UNIFORM0
    {
        u32 VALUE0;
    }
    
    uniform UNIFORM1
    {
        u32 VALUE1;
    }
    
    uniform UNIFORM2
    {
        u32 VALUE2;
    }
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getUniforms(asm)).toBe(
`MgBeginShaderUniforms
  MgBeginUniform UNIFORM0, 1
    .align 2
    VALUE0: .ds.b 4
  MgEndUniform

  MgBeginUniform UNIFORM1, 2
    .align 2
    VALUE1: .ds.b 4
  MgEndUniform

  MgBeginUniform UNIFORM2, 3
    .align 2
    VALUE2: .ds.b 4
  MgEndUniform

MgEndShaderUniforms`);
  });

  test('Negative binding number', async () => {
    const src = `
    uniform<-3> UNIFORM0
    {
        u32 VALUE0;
    }
    
    shader testshader()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Uniform binding number must be in \[0, 2\^32\)!/);
  });

  test('Invalid binding number', async () => {
    const src = `
    uniform<wrong> UNIFORM0
    {
        u32 VALUE0;
    }
    
    shader testshader()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Syntax error at line 2/);
  });

  test('Empty uniform', async () => {
    const {asm, warn} = await transpileSource(`
    uniform<0> UNIFORM0
    {
    }
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getUniforms(asm)).toBe(
`MgBeginShaderUniforms
  MgBeginUniform UNIFORM0, 0
  MgEndUniform

MgEndShaderUniforms`);
  });

  test('Extern value', async () => {
    const {asm, warn} = await transpileSource(`
    uniform<0> UNIFORM0
    {
      u32 VALUE0;
      extern u32 VALUE1;
    }
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getUniforms(asm)).toBe(
`MgBeginShaderUniforms
  MgBeginUniform UNIFORM0, 0
    .align 2
    VALUE0: .ds.b 4
  MgEndUniform

MgEndShaderUniforms`);
  });

  test('No uniforms', async () => {
    const {asm, warn} = await transpileSource(`
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getUniforms(asm)).toBe(
`MgBeginShaderUniforms
MgEndShaderUniforms`);
  });

  test('Uniform in non-magma mode', async () => {
    const src = `
    uniform<0> UNIFORM0
    {
        u32 VALUE0;
    }`;
    await expect(() => transpileSource(src, {rspqWrapper: false}))
      .rejects.toThrowError(/Uniforms are only allowed when compiling for magma \(pass '--magma' on the command line\)!/);
  });
});

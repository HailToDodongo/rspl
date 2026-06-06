import {transpileSource} from "../../lib/transpiler";

const CONF = {magma: true};

const getShader = asm => {
  const idxEndUniforms = asm.indexOf("MgEndShaderUniform");
  const idxBegin = asm.indexOf("MgBeginShader", idxEndUniforms);
  const endKeyword = "MgEndShader";
  const idxEnd = asm.indexOf(endKeyword, idxBegin);
  return asm.substring(idxBegin, idxEnd + endKeyword.length);
}

describe('Shaders', () =>
{
  test('Empty shader', async () => {
    const {asm, warn} = await transpileSource(`
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getShader(asm)).toBe(
`MgBeginShader
  j RSPQ_Loop
  nop

MgEndShader`);
  });

  test('Simple shader', async () => {
    const {asm, warn} = await transpileSource(`
    shader testshader()
    {
      u32<$a0> ptr;
      u32<$t0> value = 0x100;
      store(value, ptr);
    }`, CONF);

    expect(warn).toBe("");
    expect(getShader(asm)).toBe(
`MgBeginShader
  addiu $t0, $zero, 256
  sw $t0, ($a0)
  j RSPQ_Loop
  nop

MgEndShader`);
  });

  test('Function before shader', async () => {
    const {asm, warn} = await transpileSource(`
    function test_function(u32<$s0> ptr)
    {
      u32<$t0> value = 1;
      store(value, ptr);
    }

    shader testshader()
    {
      u32<$s0> ptr = 0x100;
      test_function(ptr);
    }`, CONF);

    expect(warn).toBe("");
    expect(getShader(asm)).toBe(
`MgBeginShader
  addiu $s0, $zero, 256
  jal test_function
  nop
  j RSPQ_Loop
  nop
test_function:
  addiu $t0, $zero, 1
  sw $t0, ($s0)
  jr $ra
  nop

MgEndShader`);
  });

  test('Arguments in shader', async () => {
    const src = `
    shader test_shader(u32 arg)
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Shaders must not specify arguments!/);
  });

  test('Result type in shader', async () => {
    const src = `
    shader<$t0> test_shader()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Shaders must not specify a result-type \(use 'shader' without `< >`\)!/);
  });

  test('Missing shader', async () => {
    const src = `
    function test_missing_shader()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Exactly one shader must be defined when compiling for magma \(use 'shader'\)!/);
  });

  test('Multiple shaders', async () => {
    const src = `
    shader test_shader1()
    {
    }

    shader test_shader2()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/A shader has already been defined!/);
  });

  test('Command in magma mode', async () => {
    const src = `
    command<0> test_command()
    {
    }

    shader testshader()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Commands must not be defined when compiling for magma \(define a 'shader' instead\)!/);
  });

  test('Shader in non-magma mode', async () => {
    const src = `
    shader testshader()
    {
    }`;
    await expect(() => transpileSource(src, {rspqWrapper: false}))
      .rejects.toThrowError(/Shaders are only allowed when compiling for magma \(pass '--magma' on the command line\)!/);
  });
});

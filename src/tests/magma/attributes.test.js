import {transpileSource} from "../../lib/transpiler";

const CONF = {magma: true};

const getAttributes = asm => {
  const idxBegin = asm.indexOf("MgBeginVertexInput");
  const endKeyword = "MgEndVertexInput";
  const idxEnd = asm.indexOf(endKeyword);
  return asm.substring(idxBegin, idxEnd + endKeyword.length);
}

const getAttributesAndShader = asm => {
  const idxBegin = asm.indexOf("MgBeginVertexInput");
  const endKeyword = "MgEndShader";
  const idxEnd = asm.indexOf(endKeyword, idxBegin);
  return asm.substring(idxBegin, idxEnd + endKeyword.length);
}

describe('Attributes', () =>
{
  test('Unused attribute', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> u32 ATTRIBUTE0;
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 0
  MgEndVertexAttribute

MgEndVertexInput`);
  });

  test('Scalar loader', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> u32 ATTRIBUTE0;
    
    shader testshader()
    {
      u32<$t0> vtx;
      u32<$t1> attr0;
      @AttrLoader("ATTRIBUTE0") attr0 = load(vtx);
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 0
    MgVertexAttributeLoaders LOAD_ATTRIBUTE08
  MgEndVertexAttribute

MgEndVertexInput`);
    expect(asm).toContain("LOAD_ATTRIBUTE08: lw $t1, 0($t0)");
  });

  test('Vector loader', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> vec16 ATTRIBUTE0;
    
    shader testshader()
    {
      u32<$t0> vtx;
      vec16<$v01> attr0;
      @AttrLoader("ATTRIBUTE0") attr0 = load(vtx);
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 0
    MgVertexAttributeLoaders LOAD_ATTRIBUTE08
  MgEndVertexAttribute

MgEndVertexInput`);
    expect(asm).toContain("LOAD_ATTRIBUTE08: lqv $v01, 0, 0, $t0");
  });

  test('Loader on declaration', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> u32 ATTRIBUTE0;
    
    shader testshader()
    {
      u32<$t0> vtx;
      @AttrLoader("ATTRIBUTE0") u32<$t1> attr0 = load(vtx);
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 0
    MgVertexAttributeLoaders LOAD_ATTRIBUTE07
  MgEndVertexAttribute

MgEndVertexInput`);
    expect(asm).toContain("LOAD_ATTRIBUTE07: lw $t1, 0($t0)");
  });

  test('Multiple loaders', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> u32 ATTRIBUTE0;
    
    shader testshader()
    {
      u32<$t0> vtx;
      @AttrLoader("ATTRIBUTE0") u32<$t1> attr0 = load(vtx);
      vec16<$v01> attr1;
      @AttrLoader("ATTRIBUTE0") attr1.xy = load(vtx).xy;
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 0
    MgVertexAttributeLoaders LOAD_ATTRIBUTE07, LOAD_ATTRIBUTE09
  MgEndVertexAttribute

MgEndVertexInput`);
    expect(asm).toContain("LOAD_ATTRIBUTE07: lw $t1, 0($t0)");
    expect(asm).toContain("LOAD_ATTRIBUTE09: llv $v01, 0, 0, $t0");
  });

  test('Optional attribute', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> u32 ATTRIBUTE0?;
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 1
  MgEndVertexAttribute

MgEndVertexInput`);
  });

  test('Patch', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> u32 ATTRIBUTE0?;
    
    shader testshader()
    {
      u32<$t0> a;
      @AttrPatch("ATTRIBUTE0:nop") a = 1;
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 1
    MgBeginVertexAttributePatch PATCH_ATTRIBUTE07
      nop
    MgEndVertexAttributePatch
  MgEndVertexAttribute

MgEndVertexInput`);
    expect(asm).toContain("PATCH_ATTRIBUTE07: addiu $t0, $zero, 1");
  });

  test('Patch (missing replacement)', async () => {
    const src = `
    attribute<0> u32 ATTRIBUTE0?;
    
    shader testshader()
    {
      u32<$t0> a;
      @AttrPatch("ATTRIBUTE0:") a = 1;
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/line 7: Annotation 'AttrPatch' must contain an attribute name and replacement instruction separated by ':'!/);
  });

  test('Patch (missing colon)', async () => {
    const src = `
    attribute<0> u32 ATTRIBUTE0?;
    
    shader testshader()
    {
      u32<$t0> a;
      @AttrPatch("ATTRIBUTE0") a = 1;
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/line 7: Annotation 'AttrPatch' must contain an attribute name and replacement instruction separated by ':'!/);
  });

  test('Multiple patches', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> u32 ATTRIBUTE0?;
    
    shader testshader()
    {
      u32<$t0> a, b;
      @AttrPatch("ATTRIBUTE0:nop") a = 1;
      @AttrPatch("ATTRIBUTE0:addiu $t1, $zero, $zero") b = a + a;
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 1
    MgBeginVertexAttributePatch PATCH_ATTRIBUTE07
      nop
    MgEndVertexAttributePatch
    MgBeginVertexAttributePatch PATCH_ATTRIBUTE08
      addiu $t1, $zero, $zero
    MgEndVertexAttributePatch
  MgEndVertexAttribute

MgEndVertexInput`);
    expect(asm).toContain("PATCH_ATTRIBUTE07: addiu $t0, $zero, 1");
    expect(asm).toContain("PATCH_ATTRIBUTE08: addu $t1, $t0, $t0");
  });

  test('Multiple attributes', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<0> u32 ATTRIBUTE0;
    attribute<1> u32 ATTRIBUTE1?;
    attribute<2> u32 ATTRIBUTE2?;
    
    shader testshader()
    {
      u32<$s0> vtx;
      u32<$t0> a, b, c;
      @AttrLoader("ATTRIBUTE0") a = load(vtx);
      @AttrLoader("ATTRIBUTE1") b = load(vtx);
      @AttrLoader("ATTRIBUTE2") c = load(vtx);
      @AttrPatch("ATTRIBUTE1:nop") a = 1;
      @AttrLoader("ATTRIBUTE1") b = load(vtx);
      @AttrLoader("ATTRIBUTE2") c = load(vtx);
      @AttrPatch("ATTRIBUTE1:nop") b = a + a;
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 0, 0
    MgVertexAttributeLoaders LOAD_ATTRIBUTE010
  MgEndVertexAttribute

  MgBeginVertexAttribute 1, 1
    MgVertexAttributeLoaders LOAD_ATTRIBUTE111, LOAD_ATTRIBUTE114
    MgBeginVertexAttributePatch PATCH_ATTRIBUTE113
      nop
    MgEndVertexAttributePatch
    MgBeginVertexAttributePatch PATCH_ATTRIBUTE116
      nop
    MgEndVertexAttributePatch
  MgEndVertexAttribute

  MgBeginVertexAttribute 2, 1
    MgVertexAttributeLoaders LOAD_ATTRIBUTE212, LOAD_ATTRIBUTE215
  MgEndVertexAttribute

MgEndVertexInput`);
    expect(asm).toContain("LOAD_ATTRIBUTE010: lw $t0, 0($s0)");
    expect(asm).toContain("LOAD_ATTRIBUTE111: lw $t1, 0($s0)");
    expect(asm).toContain("LOAD_ATTRIBUTE114: lw $t1, 0($s0)");
    expect(asm).toContain("LOAD_ATTRIBUTE212: lw $t2, 0($s0)");
    expect(asm).toContain("LOAD_ATTRIBUTE215: lw $t2, 0($s0)");
    expect(asm).toContain("PATCH_ATTRIBUTE113: addiu $t0, $zero, 1");
    expect(asm).toContain("PATCH_ATTRIBUTE116: addu $t1, $t0, $t0");
  });

  test('Arbitrary input numbers', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<65> u32 ATTRIBUTE0;
    attribute<6> u32 ATTRIBUTE1;
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 65, 0
  MgEndVertexAttribute

  MgBeginVertexAttribute 6, 0
  MgEndVertexAttribute

MgEndVertexInput`);
  });

  test('Omitted input numbers', async () => {
    const {asm, warn} = await transpileSource(`
    attribute<1> u32 ATTRIBUTE0;
    attribute u32 ATTRIBUTE1;
    attribute u32 ATTRIBUTE2;
    
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
  MgBeginVertexAttribute 1, 0
  MgEndVertexAttribute

  MgBeginVertexAttribute 2, 0
  MgEndVertexAttribute

  MgBeginVertexAttribute 3, 0
  MgEndVertexAttribute

MgEndVertexInput`);
  });

  test('Negative input number', async () => {
    const src = `
    attribute<-2> u32 ATTRIBUTE0;
    
    shader testshader()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Attribute input number must be in \[0, 2\^32\)!/);
  });

  test('Negative input number', async () => {
    const src = `
    attribute<69347592054634> u32 ATTRIBUTE0;
    
    shader testshader()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Attribute input number must be in \[0, 2\^32\)!/);
  });

  test('Invalid input number', async () => {
    const src = `
    attribute<wrong> u32 ATTRIBUTE0;
    
    shader testshader()
    {
    }`;
    await expect(() => transpileSource(src, CONF))
      .rejects.toThrowError(/Syntax error at line 2/);
  });

  test('No attributes', async () => {
    const {asm, warn} = await transpileSource(`
    shader testshader()
    {
    }`, CONF);

    expect(warn).toBe("");
    expect(getAttributes(asm)).toBe(
`MgBeginVertexInput
MgEndVertexInput`);
  });

  test('Attribute in non-magma mode', async () => {
    const src = `
    attribute<0> u32 ATTRIBUTE0;
    `;
    await expect(() => transpileSource(src, {rspqWrapper: false}))
      .rejects.toThrowError(/Attributes are only allowed when compiling for magma \(pass '--magma' on the command line\)!/);
  });
});

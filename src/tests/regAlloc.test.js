import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

// Auto-allocated variables avoid registers that the same block still needs later

async function asmFor(src) {
  const {asm, warn} = await transpileSource(src, CONF);
  expect(warn).toBe("");
  return asm;
}

async function errorFor(src) {
  try {
    await transpileSource(src, CONF);
  } catch(e) {
    return e.message;
  }
  return "";
}

describe('RegAlloc', () =>
{
  test('auto vars avoid a register needed later in the block', async () => {
    const asm = await asmFor(`function callee(u32<$t0> arg);
function test()
{
  u32 counter;
  u32 limit;
  loop {
    u32<$t0> arg;
    arg = counter;
    callee(arg);
    counter += 1;
  } while(counter != limit)
}`);

    expect(asm).toContain("or $t0, $zero, $t1");
    expect(asm).toContain("addiu $t1, $t1, 1");
  });

  test('a sibling block does not reserve anything', async () => {
    const asm = await asmFor(`function test()
{
  u32<$t5> c;
  loop {
    u32 temp;
    temp = c;
  } while(c != c)
  loop {
    u32<$t0> arg;
    arg = c;
  } while(c != c)
}`);

    expect(asm).toContain("or $t0, $zero, $t5");
  });

  test('both halves of a vec32 are kept clear', async () => {
    const asm = await asmFor(`function test()
{
  u32<$t5> c;
  loop {
    vec32 auto32;
    vec16<$v01> pinned;
    auto32 += auto32;
    pinned += pinned;
  } while(c != c)
}`);

    expect(asm).toContain("vaddc $v03, $v03, $v03.v");
    expect(asm).toContain("vadd $v02, $v02, $v02.v");
  });

  test('a vec32 takes two non-adjacent registers when no pair is free', async () => {
    let src = "function test()\n{\n";
    for(let v = 1; v <= 28; ++v) {
      if(v === 2 || v === 11) continue;
      const n = String(v).padStart(2, "0");
      src += `  vec16<$v${n}> p${n};\n`;
    }
    src += "  vec32 auto32;\n  auto32 += auto32;\n}\n";

    const asm = await asmFor(src);
    expect(asm).toContain("vaddc $v11, $v11, $v11.v");
    expect(asm).toContain("vadd $v02, $v02, $v02.v");
  });

  test('falls back when nothing else is free', async () => {

    let src = "function test()\n{\n";
    for(const r of ["t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "k0", "k1",
                    "sp", "fp", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7"]) {
      src += "  u32<$" + r + "> p_" + r + ";\n";
    }
    src += "  u32 autoVar;\n  autoVar = p_t0;\n";
    src += "  u32<$t9> late;\n  late = p_t0;\n}\n";

    const err = await errorFor(src);

    expect(err).toContain("already used for variable 'autoVar'");
    expect(err).not.toContain("Out of free registers");
  });

  test('running out of registers says so', async () => {

    const err = await errorFor(`command<0> test(u32 a)
{
  vec32 m0, m1, m2, m3;
  vec16 n0, n1, n2;
  vec16 nm, ns;
  vec16 gb;
  vec32 scr;
  vec16<$v12> screenOffset;
  vec16<$v10> normScaleW;
  vec16<$v09> uvGenArgs;
  vec16<$v08> pos;
  vec16<$v07> norm;
  vec32<$v05> posClip;
  vec16<$v04> color;
  vec16 extra;
  {
    vec16<$v02> lightDirVec;
    vec16<$v01> lightDirScale;
    vec16<$v03> uv;
    vec16<$v11> oldNorm;
    oldNorm += lightDirVec;
    uv += lightDirScale;
  }
}`);

    expect(err).toContain("already used for variable 'extra'");
    expect(err).toContain("out of vector registers");
  });
});

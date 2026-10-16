import {transpileSource} from "../../../lib/transpiler";

const DET_SRC = `state { u32 FOO; vec16 BAR; }
function test()
{
  u32<$t0> a = load(FOO);
  u32<$t1> b = load(FOO);
  vec16<$v01> v1 = load(BAR).xyzwxyzw;
  vec16<$v02> v2 = v1 * v1.x;
  vec16<$v03> v3 = v2 + v1;
  a += 2;
  b += a;
  store(v3, BAR);
  store(a, FOO);
  store(b, FOO, 4);
}`;

async function runDet(seed) {

  const {asm} = await transpileSource(DET_SRC, {
    rspqWrapper: false, reorder: true, optimize: true,
    optWorkers: 2, optSeed: seed, optIters: 4,
    optimizeTime: 200,
  });
  return asm;
}

describe('Optimizer - Determinism', () =>
{
  test('same seed, same output', async () => {
    const a = await runDet(1234);
    const b = await runDet(1234);
    expect(a).not.toBe("");
    expect(a).toBe(b);
  }, 60_000);

  test('repeated runs stay stable', async () => {
    const first = await runDet(99);
    for(let i = 0; i < 2; ++i) {
      expect(await runDet(99)).toBe(first);
    }
  }, 60_000);
});

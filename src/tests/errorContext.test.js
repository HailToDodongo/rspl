import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

async function errorFor(src) {
  try {
    await transpileSource(src, CONF);
  } catch(e) {
    return e.message;
  }
  return "";
}

describe('ErrorContext', () =>
{
  test('quotes the line and its neighbours', async () => {
    const err = await errorFor(`function test()
{
  vec32<$v05> a;
  vec16<$v05> b;
  a += a;
}`);

    expect(err).toContain("already used for variable 'a'");
    expect(err).toContain(" > 4 |   vec16<$v05> b;");
    expect(err).toContain("   3 |   vec32<$v05> a;");
    expect(err).toContain("   5 |   a += a;");
  });

  test('indentation is preserved', async () => {
    const err = await errorFor(`function test()
{
  u32<$t1> c;
  loop {
        vec32<$v05> a;
        vec16<$v05> b;
  } while(c != c)
}`);

    expect(err).toContain(" > 6 |         vec16<$v05> b;");
    expect(err).toContain("   5 |         vec32<$v05> a;");
  });

  test('clamps at the start of the file', async () => {
    const err = await errorFor(`u32<$t0> dup;
u32<$t0> dup2;
function test() { }`);

    expect(err).not.toBe("");
    expect(err).toContain(" > 2 | u32<$t0> dup2;");
  });
});

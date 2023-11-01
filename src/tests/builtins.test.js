import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Builtins', () =>
{
  test('int() - basic', () => {
    const {asm, warn} = transpileSource(`function test() {
        vec32 v0;
        vec16 v1;
        u32 a = int(v0).y;
        u32 b = int(v1).y;
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  mfc2 $t0, $v01.e1
  mfc2 $t1, $v03.e1
  jr $ra
  nop`);
  });

  test('int() - invalid (no swizzle)', () => {
    const src = `function test() {
        vec32 v0; u32 a = int(v0);
      }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 2: Builtin int\(\) requires swizzling!/);
  });

  test('int() - invalid (wrong swizzle)', () => {
    const src = `function test() {
        vec32 v0; 
        u32 a = int(v0).xyzwxyzw;
      }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 3: Builtin int\(\) swizzle must use a single element/);
  });

  test('int() - invalid (wrong type)', () => {
    const src = `function test() {
        u32 v0; 
        u32 a = int(v0).x;
      }`;

   expect(() => transpileSource(src, CONF))
    .toThrowError(/line 3: The argument of builtin int\(\) must be a vector/);
  });
});
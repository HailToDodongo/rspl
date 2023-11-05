import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Builtins', () =>
{
  test('swap() - scalar', () => {
    const {asm, warn} = transpileSource(`function test() {
        u32 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  xor $t0, $t0, $t1
  xor $t1, $t0, $t1
  xor $t0, $t0, $t1
  jr $ra
  nop`);
  });

  test('swap() - vec16', () => {
    const {asm, warn} = transpileSource(`function test() {
        vec16 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v01, $v01, $v02
  vxor $v02, $v01, $v02
  vxor $v01, $v01, $v02
  jr $ra
  nop`);
  });

  test('swap() - vec32', () => {
    const {asm, warn} = transpileSource(`function test() {
        vec32 v0, v1;
        swap(v0, v1);
      }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vxor $v01, $v01, $v03
  vxor $v03, $v01, $v03
  vxor $v01, $v01, $v03
  vxor $v02, $v02, $v04
  vxor $v04, $v02, $v04
  vxor $v02, $v02, $v04
  jr $ra
  nop`);
  });

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
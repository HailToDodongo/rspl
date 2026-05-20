import {preprocess} from "../../lib/preproc/preprocess";
import {transpileSource} from "../../lib/transpiler";
const CONF = {rspqWrapper: false};
const CONF_WRAP = {rspqWrapper: true};

describe('Preproc - Base', () =>
{
  test('Define - Basic', () => {
    const src = `
      #define TEST 42
      macro test() {
        u32 x = TEST;
      }
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      macro test() {
        u32 x = 42;
      }
    `.trim());
  });

  test('Define - Multiple', () => {
    const src = `
      #define TEST 42
      #define TEST_AB 43
      
      macro test() {
        u32 x = TEST;
        u32 y = TEST_AB;
      }
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      macro test() {
        u32 x = 42;
        u32 y = 43;
      }
    `.trim());
  });

  test('Define - Deps', () => {
    const src = `
      #define TEST 42
      #define TEST_AB TEST+1
      
      macro test() {
        u32 x = TEST;
        u32 y = TEST_AB;
      }
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      macro test() {
        u32 x = 42;
        u32 y = 42+1;
      }
    `.trim());
  });

  test('Define - Partial', () => {
    const src = `
      #define my 42
      
      macro my_function() {
        u32 x = my;
      }
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      macro my_function() {
        u32 x = 42;
      }
    `.trim());
  });

  test('Define - Undef', () => {
    const src = `
      #define TEST 42
      macro test() {
        u32 x = TEST;
      }
      #undef TEST
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      macro test() {
        u32 x = 42;
      }
    `.trim());
  });

  test('Define - Undef Before usage', () => {
    const src = `
      #define TEST 42
      #undef TEST
      
      macro test() {
        u32 x = TEST;
      }
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      macro test() {
        u32 x = TEST;
      }
    `.trim());
  });

  test('Define - Empty', () => {
    const src = `
      #define 
      macro test() {
        u32 x = TEST;
      }
    `;
    expect(() => preprocess(src, CONF))
      .toThrowError("Line 2: Invalid #define statement!");
  });

  test('Ifdef - Basic', () => {
    const src = `
      #define TEST 42
     
      #ifdef TEST2
        macro test2() {}
      #endif
      
      #ifdef TEST
        macro test() {}
      #endif
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      macro test() {}
    `.trim());
  });

  test('Ifdef - Else', () => {
    const src = `
      #define TEST 42
     
      #ifdef TEST2
        macro test2() {}
      #else
        macro test() {}
      #endif
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      macro test() {}
    `.trim());
  });

  test('Ifdef - define (true)', () => {
    const src = `
      #define TEST 42
     
      #ifdef TEST
        #define VAL 1
      #else
        #define VAL 2
      #endif
      VAL
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      1
    `.trim());
  });

  test('Ifdef - define (false)', () => {
    const src = `
      #define TEST 42
     
      #ifdef TEST_OTHER
        #define VAL 1
      #else
        #define VAL 2
      #endif
      VAL
    `;
    const res = preprocess(src, CONF);
    expect(res.trim()).toBe(`
      2
    `.trim());
  });

  test('Ifdef - nested', () => {
    const src = `
      #ifdef TEST
        #ifdef TEST2
        #endif
      #endif
      
    `;
    expect(() => preprocess(src, CONF))
      .toThrowError("Line 3: Nested #ifdef statements are not allowed!");
  });

  test('Defines preserved in source order', async () => {
    const {asm} = await transpileSource(`
#define TRI_BUFFER_COUNT 70
#define LIGHT_COUNT 8
function test(u32 dummy)
{
  u32 x = TRI_BUFFER_COUNT;
  x += LIGHT_COUNT;
}
`, CONF_WRAP);
    expect(asm).toContain("#define TRI_BUFFER_COUNT 70");
    expect(asm).toContain("#define LIGHT_COUNT 8");
    const p1 = asm.indexOf("TRI_BUFFER_COUNT");
    const p2 = asm.indexOf("LIGHT_COUNT");
    expect(p1).toBeLessThan(p2);
  });

  test('#undef removes define from output', async () => {
    const {asm} = await transpileSource(`
#define KEEP_ME 42
#define REMOVE_ME 99
#undef REMOVE_ME
function test(u32 dummy)
{
  u32 x = KEEP_ME;
}
`, CONF_WRAP);
    expect(asm).toContain("#define KEEP_ME 42");
    expect(asm).not.toContain("REMOVE_ME");
  });

  test('Large block comments do not break define collection', async () => {
    const {asm} = await transpileSource(`/***************************************
 * Multi-line block comment
 ***************************************/
#define TEST_VALUE 42
function test(u32 dummy) {}
`, CONF_WRAP);
    expect(asm).toContain("#define TEST_VALUE 42");
  });
});
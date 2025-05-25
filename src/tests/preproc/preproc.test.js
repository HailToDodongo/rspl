import {preprocess} from "../../lib/preproc/preprocess";
const CONF = {rspqWrapper: false};

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
});
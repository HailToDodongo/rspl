import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

/**
 * Test if syntax expansion / removal of syntactic sugar works
 * This simply compares the generated ASM of the shorter and longer syntax
 */
const CASES = [
  {
    name: 'Decl+Assign - Scalar',
    srcA: `u32 a; a = 1234;`,
    srcB: `u32 a = 1234;`
  }, {
    name: 'Decl+Assign - Vector',
    srcA: `vec16 a; a = 4;`,
    srcB: `vec16 a = 4;`
  }, {
    name: 'Decl+Calc - Scalar',
    srcA: `u32 a,b,c;
           c = a + b;`,
    srcB: `u32 a,b; 
           u32 c = a + b;`
  }, {
    name: 'Decl+Calc - Scalar+Const',
    srcA: `u32 a,b,c;
           c = a + 42;`,
    srcB: `u32 a,b; 
           u32 c = a + 42;`
  }, {
    name: 'Decl+Calc - Vector',
    srcA: `vec16 a,b,c;
           c = a + b;`,
    srcB: `vec16 a,b; 
           vec16 c = a + b;`
  }, {
    name: 'Decl+Calc - Vector+Const',
    srcA: `vec16 a,b,c;
           c = a + 32;`,
    srcB: `vec16 a,b; 
           vec16 c = a + 32;`
  },

  {
    name: 'Assign+Calc - Scalar',
    srcA: `u32 a,b;
           a = a + b;
           a = a - b;
           a = a | b;
           a = a & b;
           a = a ^ b;
           a = a >> b;
           a = a >>> b;
           a = a << b;
           `,
    srcB: `u32 a,b;
           a += b;
           a -= b;
           a |= b;
           a &= b;
           a ^= b;
           a >>= b;
           a >>>= b;
           a <<= b;
           `,
  }, {
    name: 'Assign+Calc - Scalar+Const',
    srcA: `u32 a,b;
           a = a + 2;
           a = a - 2;
           a = a | 2;
           a = a & 2;
           a = a ^ 2;
           a = a >> 2;
           a = a >>> 2;
           a = a << 2;
           `,
    srcB: `u32 a,b;
           a += 2;
           a -= 2;
           a |= 2;
           a &= 2;
           a ^= 2;
           a >>= 2;
           a >>>= 2;
           a <<= 2;
           `,
  }, {
    name: 'Assign+Calc - Vector',
    srcA: `vec16 a,b;
           a = a + b;
           a = a - b;
           a = a * b;
           a |= b;
           a &= b;
           a ^= b;`,
    srcB: `vec16 a,b;
           a += b;
           a -= b;
           a *= b;
           a = a | b;
           a = a & b;
           a = a ^ b;`,
  },{
    name: 'Assign+Calc - Vector+Const',
    srcA: `vec16 a,b;
           a = a + 2;
           a = a - 2;
           a = a * 2;
           a = a >> 2;
           a = a >>> 2;
           a = a << 2;
           `,
    srcB: `vec16 a,b;
           a += 2;
           a -= 2;
           a *= 2;
           a >>= 2;
           a >>>= 2;
           a <<= 2;
           `,
  },

  {
    name: 'Multi+Calc 0 - Scalar',
    srcA: `u32 a,b,c,d;
            a = b + c;
            a = a + d;`,
    srcB: `u32 a,b,c,d;
            a = b + c + d;`,
  },{
    name: 'Multi+Calc 1 - Scalar',
    srcA: `u32 a,b,c,d;
            a = b + c;
            a = a + 4;
            a = a + d;`,
    srcB: `u32 a,b,c,d;
            a = b + c + 4 + d;`,
  }
];

describe('Syntax - Expansion', () =>
{
  for(const {name, srcA, srcB} of CASES) {
    test(name, async () => {
      const {asm: asmA, warn: warnA} = await transpileSource(`function test() { ${srcA} }`, CONF);
      const {asm: asmB, warn: warnB} = await transpileSource(`function test() { ${srcB} }`, CONF);

      expect(asmA).toContain("test:"); // some sanity check if any source was generated at all
      expect(asmB).toContain("test:");
      expect(asmA).toContain("jr $ra");
      expect(asmB).toContain("jr $ra");

      expect(warnA).toBeFalsy();
      expect(warnB).toBeFalsy();
      expect(asmA).toBe(asmB);
    });
  }
});
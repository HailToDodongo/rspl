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
  },
  {
    name: 'Multi+Calc 2 - Scalar',
    srcA: `u32 a,b,c,d;
            a = b + 8;
            a = b - 20;
            `,
    srcB: `u32 a,b,c,d;
            a = b + 4 + 4;
            a = b - (2 + 2 * 9);`,
  },
  {
    name: 'Multi+Calc 3 - Scalar',
    srcA: `u32 a,b,c,d;
            a = b + 50;
            `,
    srcB: `u32 a,b,c,d;
            a = b + ((3 + 2) * 10);`,
  },
  {
    name: 'Multi+Calc 4 - Scalar',
    srcA: `u32 a,b,c,d;
            u32 tmp = b + c;
            a = tmp >> d;
            `,
    srcB: `u32 a,b,c,d;
            a = (b + c) >> d;
            `,
  },
  {
    name: 'Multi+Calc 5 - Scalar',
    srcA: `u32 a,b,c,d;
            u32 tmp0 = b + c;
            u32 tmp1 = d - a;
            a = tmp0 >> tmp1;
            `,
    srcB: `u32 a,b,c,d;
            a = (b + c) >> (d - a);
            `,
  },
    {
    name: 'Multi+Calc 5.1 - Scalar',
    srcA: `u32 a,b,c,d;
            u32 tmp0 = c + d;
            a = a + tmp0;
            `,
    srcB: `u32 a,b,c,d;
            a = a + (c + d);
            `,
  },
  {
    name: 'Multi+Calc 6 - Vector deeply nested',
    srcA: `vec16<$v02> a,b,c,d;           
          vec16 tmp0, tmp1, tmp2, tmp3, tmp4;

          tmp1 = b * c;
          tmp0 = a + tmp1;
                             
          tmp4 = a + 4;
          tmp3 = tmp4 * c;
          tmp2 = d - tmp3;
          
          a = tmp0 - tmp2;
            `,
    srcB: `vec16<$v02> a,b,c,d;
           a = (a + b * c) - (d - (a + (2+2)) * c);
            `,
  },
  {
    name: 'Multi+Calc 7 - Scalar Increment',
    srcA: `u32 a,b,c,d;
            u32 tmp0 = b + c;
            a = a + tmp0;
            `,
    srcB: `u32 a,b,c,d;
            a += b + c;
            `,
  },
  {
    name: 'Multi+Calc - Vector + Cast',
    srcA: `vec16 a,b,c,d;
            vec16 tmp = b + c;
            a = tmp:sfract * d:sfract;
            `,
    srcB: `vec16 a,b,c,d;
            a = (b + c) * d:sfract;
            `,
  },
  {
    name: 'Multi+Calc - Scalar Const Op-Test',
    srcA: `u32 a,b,c,d;
            ARITH:
            a = b + 5;
            a = b + 6;
            a = b + -1;
            a = b + 3;
            SHIFT:
            a = b + 8;
            a = b + 4;
            a = b + -4;
            a = b + -4;
            a = b + ${-16 >>> 2};
            LOGIC:
            a = b + 0b1111;
            a = b + 0b1110;
            a = b + 0b110011;
            `,
    srcB: `u32 a,b,c,d;
            ARITH:
            a = b + (2 + 3);
            a = b + (2 * 3);
            a = b + (2 - 3);
            a = b + (10 / 3);
            SHIFT:
            a = b + (1 << 3);
            a = b + (16 >> 2);
            a = b + (-2 << 1);
            a = b + (-16 >> 2);
            a = b + (-16 >>> 2);
            LOGIC:
            a = b + (0b0101 | 0b1010);
            a = b + (0b1111 & 0b1110);
            a = b + (0b010000 ^ 0b100011);
            `,
  },
  {
    name: 'Multi+Calc - Scalar Const Order or Operations',
    srcA: `u32 a,b,c,d;
            a = b + 10;
            a = b + 1;
            a = b + 4;
            a = b + 0x14;
            `,
    srcB: `u32 a,b,c,d;
            a = b + (1 + 1 * 9);
            a = b + (10 - 1 * 9);
            a = b + (1 + 1 << 1);
            a = b + (1 + 1 << 1 | 0x10);
            `,
  },
  {
    name: 'Multi+Calc - Scalar Const Brackets',
    srcA: `u32 a,b,c,d;
            a = b + 10;
            a = b + 20;
            a = b + 10;
            a = b + 31;
            `,
    srcB: `u32 a,b,c,d;
            a = b + ((5) + (5));
            a = b + ((10) + (((5*2))));
            a = b + ((((((((((((((((10))))))))))))))));
            a = b + (3 * 10) + 1;
            `,
  },
  {
    name: 'Multi+Calc - Scalar Only',
    srcA: `u32 a,b,c,d;
            a = 10;
            a = 20;
            a = 30;
            a = 40;
            a = 100;
            a = 200;
            `,
    srcB: `u32 a,b,c,d;
            a = 5 + 5;
            a = 40 / 2;
            a = 35 - 5;
            a = 20 * 2;
            a = (10 * 5) * 2;
            a = 2 * (1 + 9 * 11);
            `,
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
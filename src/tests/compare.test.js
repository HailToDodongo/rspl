import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

describe('Comparison', () =>
{
  test('Vector (vec16 vs vec16)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a < b;  //
      res = a >= b; //
      res = a == b; //
      res = a != b; //
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vlt $v01, $v02, $v03
  ##
  vge $v01, $v02, $v03
  ##
  veq $v01, $v02, $v03
  ##
  vne $v01, $v02, $v03
  ##
  jr $ra
  nop`);
  });

  test('Vector (vec16 vs const)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = a < 0;  //
      res = a >= 2; //
      res = a == 32; //
      res = a != 256; //
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vlt $v01, $v02, $v00.e0
  ##
  vge $v01, $v02, $v30.e6
  ##
  veq $v01, $v02, $v30.e2
  ##
  vne $v01, $v02, $v31.e7
  ##
  jr $ra
  nop`);
  });

  test('Vector-Select (vec16)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec16<$v01> res, a, b;
      res = select(a, b);  //
      res = select(a, 32); //
      // res = select(64, b); // INVALID
      // res = select(2, 4);  // INVALID
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmrg $v01, $v02, $v03
  ##
  vmrg $v01, $v02, $v30.e2
  ##
  ## res = select(64, b); // INVALID
  ## res = select(2, 4);  // INVALID
  jr $ra
  nop`);
  });

  test('Vector-Select (vec32)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> res, a, b;
      res = select(a, b);  //
      res = select(a, b.y);  //
      res = select(a, 32); //
      // res = select(64, b); // INVALID
      // res = select(2, 4);  // INVALID
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmrg $v01, $v03, $v05
  vmrg $v02, $v04, $v06
  ##
  vmrg $v01, $v03, $v05.e1
  vmrg $v02, $v04, $v06.e1
  ##
  vmrg $v01, $v03, $v30.e2
  vmrg $v02, $v04, $v00.e2
  ##
  ## res = select(64, b); // INVALID
  ## res = select(2, 4);  // INVALID
  jr $ra
  nop`);
  });

  test('Vector-Select (vec32 cast)', () => {
    const {asm, warn} = transpileSource(`function test() {
      vec32<$v01> res, a, b;
      res:sint = select(a, b:sfract);  //
      res:sfract = select(a, 32); //
      // res = select(64, b); // INVALID
      // res = select(2, 4);  // INVALID
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  vmrg $v01, $v03, $v06
  ##
  vmrg $v02, $v04, $v00.e2
  ##
  ## res = select(64, b); // INVALID
  ## res = select(2, 4);  // INVALID
  jr $ra
  nop`);
  });
});
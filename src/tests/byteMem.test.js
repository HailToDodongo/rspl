import {transpileSource} from "../lib/transpiler";

const CONF = {rspqWrapper: false};

async function requireByteThrowsWith(src, msgPart) {
  let threw = false;
  try {
    await transpileSource(src, CONF);
  } catch(e) {
    threw = true;
    expect(e.message).toContain(msgPart);
  }
  expect(threw).toBe(true);
}

describe('ByteMem', () =>
{
  test('load_byte_lo/hi element from the lane', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      v.x = load_byte_hi(addr, 0x10);
      v.x = load_byte_lo(addr, 0x10);
      v.z = load_byte_hi(addr);
      v.W = load_byte_lo(addr, -5);
    }`, CONF);

    expect(warn).toBe("");
    // lane x=0, z=2, W=7 -> even element is the high byte, odd the low byte
    expect(asm).toBe(`test:
  lbv $v01, 0, 16, $t0
  lbv $v01, 1, 16, $t0
  lbv $v01, 4, 0, $t0
  lbv $v01, 15, -5, $t0
  jr $ra
  nop`);
  });

  test('store_byte_lo/hi element from the lane', async () => {
    const {asm, warn} = await transpileSource(`function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      store_byte_lo(v.x, addr, 0x10);
      store_byte_hi(v.W, addr, 63);
      store_byte_lo(v.y, addr, -64);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  sbv $v01, 1, 16, $t0
  sbv $v01, 14, 63, $t0
  sbv $v01, 3, -64, $t0
  jr $ra
  nop`);
  });

  test('memory label address goes through $at', async () => {
    const {asm, warn} = await transpileSource(`state { u8 BUFF[64]; }
function test() {
      vec16<$v01> v;
      v.y = load_byte_lo(BUFF, 4);
      store_byte_lo(v.y, BUFF);
    }`, CONF);

    expect(warn).toBe("");
    expect(asm).toBe(`test:
  ori $at, $zero, %lo(BUFF)
  lbv $v01, 3, 4, $at
  ori $at, $zero, %lo(BUFF)
  sbv $v01, 3, 0, $at
  jr $ra
  nop`);
  });

  test('errors', async () => {
    const noSwizzle = `function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      v = load_byte_lo(addr);
    }`;
    await requireByteThrowsWith(noSwizzle, "requires a single-lane swizzle");

    const multiLane = `function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      v.xy = load_byte_lo(addr);
    }`;
    await requireByteThrowsWith(multiLane, "requires a single-lane swizzle");

    // the immediate is a signed 7-bit field, unscaled
    const offHigh = `function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      v.x = load_byte_lo(addr, 64);
    }`;
    await requireByteThrowsWith(offHigh, "range -64 to 63");

    const offLow = `function test() {
      vec16<$v01> v;
      u32<$t0> addr;
      store_byte_hi(v.x, addr, -65);
    }`;
    await requireByteThrowsWith(offLow, "range -64 to 63");

    const scalarDst = `function test() {
      u32<$t1> q;
      u32<$t0> addr;
      q = load_byte_lo(addr);
    }`;
    await requireByteThrowsWith(scalarDst, "requires a vector variable");

    const scalarVal = `function test() {
      u32<$t0> addr;
      store_byte_lo(addr, addr);
    }`;
    await requireByteThrowsWith(scalarVal, "requires a vector variable");
  });
});

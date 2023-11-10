/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

export const TYPE_SIZE = {
  s8   : 1,
  u8   : 1,
  s16  : 2,
  u16  : 2,
  s32  : 4,
  u32  : 4,
  vec16: 2*8, // 16bit per lane @ 8 lanes
  vec32: 4*8, // 16bit per lane @ 8 lanes, two register (int, fract)
};

export const TYPE_ALIGNMENT = {
  s8   : 1,
  u8   : 1,
  s16  : 1,
  u16  : 1,
  s32  : 2,
  u32  : 2,
  vec16: 3,
  vec32: 4,
};

export const TYPE_REG_COUNT = {
  s8   : 1,
  u8   : 1,
  s16  : 1,
  u16  : 1,
  s32  : 1,
  u32  : 1,
  vec16: 1,
  vec32: 2,
};

export const isTwoRegType = type => type === "vec32";

export const isVecType = type => type.startsWith("vec");

export const isSigned = type => type.startsWith("s");

export const toHex = (val, pad = 2) =>
  "0x" + val.toString(16).padStart(pad, '0').toUpperCase();

export function u32InS16Range(valueU32) {
  return valueU32 <= 0x7FFF || valueU32 >= 0xFFFF8000;
}

export function u32InU16Range(valueU32) {
  return valueU32 <= 0xFFFF;
}

export function f32ToFP32(valueF32) {
  return Math.round(valueF32 * (1<<16)) >>> 0;
}
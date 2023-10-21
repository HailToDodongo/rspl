/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
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
  s16  : 2,
  u16  : 2,
  s32  : 4,
  u32  : 4,
  vec16: 8,
  vec32: 8,
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

export const isSigned = (type) => type.startsWith("s");

export const toHex = (val, pad = 4) =>
  "0x" + val.toString(16).padStart(pad, '0').toUpperCase();

export const toHexSafe = (val, pad = 4)=>
  typeof(val) === "number" ? toHex(val, pad) : val;
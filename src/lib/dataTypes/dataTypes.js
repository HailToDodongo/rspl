/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

/** @type {Record<DataType, number>} */
export const TYPE_SIZE = {
  s8   : 1,
  u8   : 1,
  s16  : 2,
  u16  : 2,
  s32  : 4,
  u32  : 4,
  vec16: 2*8, // 16bit per lane @ 8 lanes
  vec32: 4*8, // 16bit per lane @ 8 lanes, two register
};

/** @type {Record<DataType, number>} */
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

/** @type {Record<DataType, number>} */
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

export const SCALAR_TYPES = ["s8", "u8", "s16", "u16", "s32", "u32"];
export const VEC_CASTS = ["uint", "sint", "ufract", "sfract"];

/**
 * @param {DataType} type
 * @returns {boolean}
 */
export const isTwoRegType = type => type === "vec32";

/**
 * @param {DataType} type
 * @returns {boolean}
 */
export const isVecType = type => type.startsWith("vec");

/**
 * @param {string} type
 * @returns {boolean}
 */
export const isSigned = type => type.startsWith("s");

/**
 * @param {number} val
 * @param {number} pad
 * @returns {string}
 */
export const toHex = (val, pad = 2) =>
  "0x" + val.toString(16).padStart(pad, '0').toUpperCase();

/**
 * @param {number} valueU32
 * @returns {boolean}
 */
export function u32InS16Range(valueU32) {
  return valueU32 <= 0x7FFF || valueU32 >= 0xFFFF8000;
}

/**
 * @param {number} valueU32
 * @returns {boolean}
 */
export function u32InU16Range(valueU32) {
  return valueU32 <= 0xFFFF;
}

/**
 * @param {number} valueF32
 * @returns {number}
 */
export function f32ToFP32(valueF32) {
  return Math.round(valueF32 * (1<<16)) >>> 0;
}
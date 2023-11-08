/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import {REG} from "./registers.js";

export const SWIZZLE_MAP = {
  "": ".v",
  "xyzwXYZW": ".v",
  "xxzzXXZZ":  ".q0",
  "yywwYYWW":  ".q1",
  "xxxxXXXX":  ".h0",
  "yyyyYYYY":  ".h1",
  "zzzzZZZZ":  ".h2",
  "wwwwWWWW":  ".h3",
  "xxxxxxxx":  ".e0",
  "yyyyyyyy":  ".e1",
  "zzzzzzzz":  ".e2",
  "wwwwwwww":  ".e3",
  "XXXXXXXX":  ".e4",
  "YYYYYYYY":  ".e5",
  "ZZZZZZZZ":  ".e6",
  "WWWWWWWW":  ".e7",

  "x": ".e0",
  "y": ".e1",
  "z": ".e2",
  "w": ".e3",
  "X": ".e4",
  "Y": ".e5",
  "Z": ".e6",
  "W": ".e7",
};

export const SWIZZLE_MAP_KEYS_STR = Object.keys(SWIZZLE_MAP).filter(Boolean).join(", ");

export const SWIZZLE_SCALAR_IDX = {
  "x": 0, "y": 1, "z": 2, "w": 3,
  "X": 4, "Y": 5, "Z": 6, "W": 7,
};

export const POW2_SWIZZLE_VAR = {
      1: {type: 'vec16', reg: REG.VSHIFT,  swizzle: 'W'},
      2: {type: 'vec16', reg: REG.VSHIFT,  swizzle: 'Z'},
      4: {type: 'vec16', reg: REG.VSHIFT,  swizzle: 'Y'},
      8: {type: 'vec16', reg: REG.VSHIFT,  swizzle: 'X'},
     16: {type: 'vec16', reg: REG.VSHIFT,  swizzle: 'w'},
     32: {type: 'vec16', reg: REG.VSHIFT,  swizzle: 'z'},
     64: {type: 'vec16', reg: REG.VSHIFT,  swizzle: 'y'},
    128: {type: 'vec16', reg: REG.VSHIFT,  swizzle: 'x'},
    256: {type: 'vec16', reg: REG.VSHIFT8, swizzle: 'W'},
    512: {type: 'vec16', reg: REG.VSHIFT8, swizzle: 'Z'},
   1024: {type: 'vec16', reg: REG.VSHIFT8, swizzle: 'Y'},
   2048: {type: 'vec16', reg: REG.VSHIFT8, swizzle: 'X'},
   4096: {type: 'vec16', reg: REG.VSHIFT8, swizzle: 'w'},
   8192: {type: 'vec16', reg: REG.VSHIFT8, swizzle: 'z'},
  16384: {type: 'vec16', reg: REG.VSHIFT8, swizzle: 'y'},
  32768: {type: 'vec16', reg: REG.VSHIFT8, swizzle: 'x'},
};

export const isScalarSwizzle = (swizzle) => {
  return swizzle.length === 1;
}
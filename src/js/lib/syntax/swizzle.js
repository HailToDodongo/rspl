/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

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

export const isScalarSwizzle = (swizzle) => {
  return swizzle.length === 1;
}
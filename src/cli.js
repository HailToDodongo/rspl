/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import { transpileSource } from "./lib/transpiler";
import { readFileSync, writeFileSync } from "fs";

const source = readFileSync(process.argv[2], "utf8");
const pathOut = process.argv[2] + ".S";

let config = {optimize: true, rspqWrapper: true};

console.time("transpile");
const asmRes = transpileSource(source, config);
console.timeEnd("transpile");

if(asmRes.warn) {
  console.warn(asmRes.warn);
}

writeFileSync(pathOut, asmRes.asm);



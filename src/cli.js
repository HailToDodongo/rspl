/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import { transpileSource } from "./lib/transpiler";
import { readFileSync, writeFileSync } from "fs";

const source = readFileSync(process.argv[2], "utf8");
const pathOut = process.argv[2].replace(".rspl", "") + ".S";

let config = {optimize: true, reorder: false, rspqWrapper: true};

for(let i=3; i<process.argv.length; ++i) {
  if(process.argv[i] === "--reorder") {
    config.reorder = true;
  }
  if(process.argv[i] === "--optimize") {
    config.optimize = true;
  }
}

async function main() {
  console.time("transpile");
  const asmRes = await transpileSource(source, config);
  console.timeEnd("transpile");
  if(asmRes.warn) {
    console.warn(asmRes.warn);
  }
  if(asmRes.info) {
    console.info(asmRes.info);
  }
  writeFileSync(pathOut, asmRes.asm);
}

main().catch(e => {
  console.error(e);
  process.exit(1);
});
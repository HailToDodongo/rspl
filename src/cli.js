/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import { transpileSource } from "./lib/transpiler";
import { readFileSync, writeFileSync } from "fs";
import * as path from "path";

const source = readFileSync(process.argv[2], "utf8");
const pathOut = process.argv[2].replace(".rspl", "") + ".S";

let config = {
  optimize: true,
  reorder: false,
  rspqWrapper: true,
  defines: {}
};

for(let i=3; i<process.argv.length; ++i) {
  if(process.argv[i] === "--reorder") {
    config.reorder = true;
  }
  if(process.argv[i] === "--optimize") {
    config.optimize = true;
  }
  if(process.argv[i] === "-D") {
    if(!process.argv[i+1])throw new Error("Missing define name/value in arguments!");
    const [key, value] = process.argv[++i].split("=");
    config.defines[key] = value;
  }
}

async function main() {
  const sourceBaseDir = path.dirname(process.argv[2]);
  config.fileLoader =  filePath => readFileSync(path.join(sourceBaseDir, filePath), "utf8");

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
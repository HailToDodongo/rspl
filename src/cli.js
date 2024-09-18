/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import { transpileSource } from "./lib/transpiler";
import { readFileSync, writeFileSync } from "fs";
import * as path from "path";
import {initWorker, registerTask, WorkerThreads} from "./lib/workerThreads.js";

import { fileURLToPath } from 'url';
import {reorderRound, reorderTask} from "./lib/optimizer/asmOptimizer.js";
const __filename = fileURLToPath(import.meta.url);

let config = {
  optimize: true,
  optimizeTime: 1000 * 30,
  optimizeWorker: 4,
  reorder: false,
  rspqWrapper: true,
  defines: {},
  patchFunction: undefined,
};

function getFunctionStartEnd(source, funcName) {
 const funcIdx = source.search(funcName + ":\n");
  if(funcIdx === -1) {
    throw new Error("Function not found in output file!");
  }
  // search for end, each following line starts with 2 spaces and the function ends when a line has none
  const endIdx = source.substring(funcIdx).search(/\n[A-Za-z0-9]/);
  if(endIdx === -1) {
    throw new Error("Function end not found in output file!");
  }
  return [funcIdx, funcIdx+endIdx];
}

for(let i=3; i<process.argv.length; ++i) {
  if(process.argv[i] === "--reorder") {
    config.reorder = true;
  }
  if(process.argv[i] === "--optimize") {
    config.optimize = true;
  }

  if(process.argv[i].startsWith("--opt-time=")) {
    config.optimizeTime = parseInt(process.argv[i].split("=")[1]) * 1000;
  }
  if(process.argv[i].startsWith("--opt-worker=")) {
    config.optimizeWorker = parseInt(process.argv[i].split("=")[1]);
  }

  if(process.argv[i] === "--no-rspq-wrapper") {
    config.rspqWrapper = false;
  }

  if(process.argv[i] === "--patch") {
    if(!process.argv[i+1])throw new Error("Missing patch function name in arguments!");
    config.patchFunction = process.argv[++i];
  }

  if(process.argv[i] === "-D") {
    if(!process.argv[i+1])throw new Error("Missing define name/value in arguments!");
    const [key, value] = process.argv[++i].split("=");
    config.defines[key] = value;
  }
}

async function main() {
  registerTask("reorder", reorderTask);
  if(initWorker())return;

  const source = readFileSync(process.argv[2], "utf8");
  const pathOut = process.argv[2].replace(".rspl", "") + ".S";

  const selfPath = process.argv.find(arg => arg.includes(".mjs"));
  const worker = new WorkerThreads(config.optimizeWorker, selfPath);

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

  if(config.patchFunction) {
    console.log("Patching function", config.patchFunction);
    const oldSource = readFileSync(pathOut, "utf8");
    const posOld = getFunctionStartEnd(oldSource, config.patchFunction);
    const posNew = getFunctionStartEnd(asmRes.asm, config.patchFunction);
    const newSource = oldSource.substring(0, posOld[0])
      + asmRes.asm.substring(posNew[0], posNew[1])
      + oldSource.substring(posOld[1]);

    writeFileSync(pathOut, newSource);
  } else {
    writeFileSync(pathOut, asmRes.asm);
  }
  worker.stop();
}

main().catch(e => {
  console.error(e);
  process.exit(1);
});
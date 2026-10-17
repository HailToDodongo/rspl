/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import { transpileSource, patchAsmFunctions } from "./lib/transpiler";
import { readFileSync, writeFileSync, watch } from "fs";
import * as path from "path";
import {initWorker, registerTask, WorkerThreads} from "./lib/workerThreads.js";
import crypto from 'crypto';

import { fileURLToPath } from 'url';
import {reorderRound, reorderTask} from "./lib/optimizer/asmOptimizer.js";
const __filename = fileURLToPath(import.meta.url);

/**
 * @type RSPLConfig
 */
let config = {
  optimize: true,
  optimizeTime: 1000 * 30,
  optimizeWorker: 4,
  reorder: false,
  rspqWrapper: true,
  defines: {},
  patchFunctions: [],
  watch: false,
  debugInfo: true,
  magma: false,
  armips: false
};

let inputFile = undefined;

for(let i=2; i<process.argv.length; ++i) {
  if(!process.argv[i].startsWith("-")) {
    inputFile = process.argv[i];
    continue;
  }
  if(process.argv[i] === "--reorder") {
    config.reorder = true;
  }
  if(process.argv[i] === "--optimize") {
    config.optimize = true;
  }
  if(process.argv[i] === "--watch") {
    config.watch = true;
  }

  if(process.argv[i].startsWith("--opt-time=")) {
    config.optimizeTime = parseInt(process.argv[i].split("=")[1]) * 1000;
  }
  if(process.argv[i].startsWith("--opt-worker=") || process.argv[i].startsWith("--opt-workers=")) {
    config.optimizeWorker = parseInt(process.argv[i].split("=")[1]);
  }
  
  if(process.argv[i].startsWith("--opt-seed=")) {
    config.optSeed = parseInt(process.argv[i].split("=")[1]);
  }
  if(process.argv[i].startsWith("--opt-iters=")) {
    config.optIters = parseInt(process.argv[i].split("=")[1]);
  }

  if(process.argv[i] === "--no-rspq-wrapper") {
    config.rspqWrapper = false;
  }

  if(process.argv[i] === "--patch") {
    if(!process.argv[i+1])throw new Error("Missing patch function name in arguments!");
    config.patchFunctions = process.argv[++i].split(",");
  }

  if(process.argv[i] === "-D") {
    if(!process.argv[i+1])throw new Error("Missing define name/value in arguments!");
    const [key, value] = process.argv[++i].split("=");
    config.defines[key] = value ?? "";
  }

  if (process.argv[i] === "-o") {
    if(!process.argv[i+1])throw new Error("Missing output path in arguments!");
    config.output = process.argv[++i]
  }

  if(process.argv[i] === "--magma") {
    config.magma = true;
  }
  
  if(process.argv[i] === "--armips") {
    config.armips = true;
  }
}

if(!inputFile) {
  console.error("Usage: rspl <input.rspl> [-o <output.S>] [--reorder] [--optimize] [--watch] [--armips] [--magma]\n"
    + "            [--no-rspq-wrapper] [--patch <func,...>] [-D <name>=<value>]\n"
    + "            [--opt-time=<sec>] [--opt-seed=<n>] [--opt-iters=<n>]");
  process.exit(1);
}

let lastFileHash = "";

async function main() {
  registerTask("reorder", reorderTask);
  if(initWorker())return;

  const source = readFileSync(inputFile, "utf8");

  const fileHash = crypto.createHash('md5').update(source).digest('hex');
  if(fileHash === lastFileHash) {
    console.log("File unchanged, skipping transpile.");
    return;
  }
  lastFileHash = fileHash;

  const pathOut = config.output ?? inputFile.replace(".rspl", "") + ".S";

  const selfPath = process.argv.find(arg => arg.includes(".mjs"));
  //const worker = new WorkerThreads(config.optimizeWorker, selfPath);

  const sourceBaseDir = path.dirname(inputFile);
  config.fileLoader =  filePath => readFileSync(path.join(sourceBaseDir, filePath), "utf8");

  const finalizeASM = (asmRes, lastRun = false) => {
    if(asmRes.warn && lastRun) {
      console.warn(asmRes.warn);
      asmRes.warn = "";
    }
    if(asmRes.info && lastRun) {
      console.info(asmRes.info);
      asmRes.info = "";
    }

    if(config.patchFunctions.length) {
      if(lastRun)console.log("Patching functions", config.patchFunctions.join(", "));
      const oldSource = readFileSync(pathOut, "utf8");
      writeFileSync(pathOut, patchAsmFunctions(oldSource, asmRes.asm, config.patchFunctions));
    } else {
      writeFileSync(pathOut, asmRes.asm);
    }
  };

  console.time("transpile");
  const asmRes = await transpileSource(source, config, finalizeASM);
  console.timeEnd("transpile");

  finalizeASM(asmRes, true);

  //worker.stop();
}

if(config.watch) {
  console.log("Watching for changes on: " + inputFile);
  let lastChangeTime = 0;
  let isRunning = false;
  watch(inputFile, (eventType, filename) =>
  {
    if(isRunning || eventType !== "change")return;
    if(Date.now() - lastChangeTime < 100)return;
    lastChangeTime = Date.now();
    isRunning = true;
    main().catch(e => console.error(e))
      .finally(() => isRunning = false);
  });
} else {
  main().catch(e => {
    console.error(e);
    process.exit(1);
  });
}
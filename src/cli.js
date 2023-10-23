/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import { Parser, Grammar } from "nearley";
import grammarDef from './lib/grammar';
import { transpile } from "./lib/transpiler";
import { readFileSync, writeFileSync } from "fs";

const source = readFileSync(process.argv[2], "utf8");
const pathOut = process.argv[2] + ".S";

const parser = new Parser(Grammar.fromCompiled(grammarDef));

console.time("parse");
const res = parser.feed(source);
console.timeEnd("parse");

if(res.results.length > 1) {
  throw Error("Warning: ambiguous syntax!");
}

console.time("transpile");
const asm = transpile(res.results[0]);
console.timeEnd("transpile");

writeFileSync(pathOut, asm);



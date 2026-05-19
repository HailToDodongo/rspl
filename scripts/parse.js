/**
 * RSPL parser wrapper — reads .rspl source, outputs AST as JSON to stdout.
 *
 * Usage:
 *   node scripts/parse.js <input.rspl>           # full parse (preproc + parse)
 *   node scripts/parse.js --preprocessed <file>   # skip preprocessor
 *
 * This is the thin JS-side of the C++ port. It keeps the existing Nearley
 * grammar and serializes the resulting AST as JSON.
 */

import { readFileSync } from "fs";
import * as path from "path";
import nearly from "nearley";
import grammarDef from "../src/lib/grammar.cjs";
import { stripComments } from "../src/lib/preproc/preprocess.js";
import { preprocess } from "../src/lib/preproc/preprocess.js";

const grammar = nearly.Grammar.fromCompiled(grammarDef);

function fileLoader(filePath) {
  const sourceBaseDir =
      process.env.RSPL_SOURCE_DIR || path.dirname(args.inputFile || ".");
  return readFileSync(path.join(sourceBaseDir, filePath), "utf8");
}

const args = {
  inputFile: null,
  skipPreproc: false,
};

for (let i = 2; i < process.argv.length; ++i) {
  if (process.argv[i] === "--preprocessed") {
    args.skipPreproc = true;
  } else if (process.argv[i].startsWith("-")) {
    // unknown flag — ignore
  } else {
    args.inputFile = process.argv[i];
  }
}

function parse(source) {
  const defines = {};
  if (args.skipPreproc) {
    // Source already has comments stripped and defines expanded by C++
    source = stripComments(source); // strip comments again for safety
  } else {
    source = stripComments(source);

    if (process.env.RSPL_DEFINES) {
      for (const def of process.env.RSPL_DEFINES.split(",")) {
        const [key, value] = def.split("=");
        if (key) {
          defines[key] = {
            regex: new RegExp(`\\b${key}\\b`, "g"),
            value: value || "1",
          };
        }
      }
    }
    source = preprocess(source, defines, fileLoader);
  }

  const parser = new nearly.Parser(grammar);
  const astList = parser.feed(source);

  if (astList.results.length > 1) {
    throw new Error("Warning: ambiguous syntax!");
  }

  const ast = astList.results[0];
  if (process.env.RSPL_KEEP_DEFINES && !args.skipPreproc) {
    ast.defines = defines;
  } else {
    delete ast.defines;
  }

  return ast;
}

function main() {
  if (!args.inputFile) {
    console.error("Usage: node parse.js [--preprocessed] <input.rspl>");
    process.exit(1);
  }

  const source = readFileSync(args.inputFile, "utf8");
  const ast = parse(source);
  process.stdout.write(JSON.stringify(ast, null, 2));
}

main();

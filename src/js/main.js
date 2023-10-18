/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import { Parser, Grammar } from "nearley";
import grammarDef from './grammar';

// Syntax Highlighting
//import 'highlight.js/styles/github.css';
import hljs from 'highlight.js/lib/core';
import mipsasm from 'highlight.js/lib/languages/mipsasm';
import json from 'highlight.js/lib/languages/json';

import { transpile } from "./lib/transpiler";

// Restore old code from last session
const oldCode = localStorage.getItem("lastCode") || "";
inputRSPL.value = oldCode;

// Register code-highlighting
hljs.registerLanguage('mipsasm', mipsasm);
hljs.registerLanguage('json', json);
hljs.highlightAll();

function writeAST(data) {
  outputAST.textContent = data;
  delete outputAST.dataset.highlighted;
  hljs.highlightElement(outputAST);
}

function writeASM(data) {
  outputASM.textContent = data;
  delete outputASM.dataset.highlighted;
  hljs.highlightElement(outputASM);
}

function update()
{
  try {
    const parser = new Parser(Grammar.fromCompiled(grammarDef));
    const res = parser.feed(inputRSPL.value);  
    outputError.innerHTML = "";

    writeAST(JSON.stringify(res.results, null, 2));
    outputError.innerHTML += "Code parsed without errors.\n";

    const asm = transpile(res.results);
    outputError.innerHTML += "Transpiled successfully!\n";
    writeASM(asm);

    localStorage.setItem("lastCode", inputRSPL.value);
  } catch(e) {
    console.error(e.message);
    outputError.innerHTML = e.message;
  }

};

inputRSPL.oninput = () => update();
update();

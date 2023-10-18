import { Parser, Grammar } from "nearley";
import grammarDef from './grammar';

// Syntax Highlighting
//import 'highlight.js/styles/github.css';
import hljs from 'highlight.js/lib/core';
import mipsasm from 'highlight.js/lib/languages/mipsasm';
import json from 'highlight.js/lib/languages/json';
 
// Restore old code from last session
const oldCode = localStorage.getItem("lastCode") || "";
inputRSPL.value = oldCode;

// Register code-highlighting
hljs.registerLanguage('mipsasm', mipsasm);
hljs.registerLanguage('json', json);
hljs.highlightAll();

function writeAST(data) {
  outputAST.innerHTML = data;
  delete outputAST.dataset.highlighted;
  hljs.highlightElement(outputAST);
}

function update()
{
  try {
    const parser = new Parser(Grammar.fromCompiled(grammarDef));
    const res = parser.feed(inputRSPL.value);  

    writeAST(JSON.stringify(res.results, null, 2));
    outputError.innerHTML = "Transpiled successfully!";

    localStorage.setItem("lastCode", inputRSPL.value);
  } catch(e) {
    console.error(e.message);
    outputError.innerHTML = e.message;
  }

};

inputRSPL.oninput = () => update();
update();

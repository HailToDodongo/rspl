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

import rsplHighlightRules from './editorMode/rspl_highlight_rules';
import modeGLSL from 'ace-builds/src-min-noconflict/mode-glsl';

import { edit as aceEdit } from "ace-builds";
import "ace-builds/src-min-noconflict/theme-tomorrow_night_eighties";
import "ace-builds/src-min-noconflict/mode-glsl";

import { transpile } from "./lib/transpiler";

// Restore old code from last session
const oldSource = localStorage.getItem("lastCode") || "";

// Register code-highlighting
hljs.registerLanguage('mipsasm', mipsasm);
hljs.registerLanguage('json', json);
hljs.highlightAll();

const editor = aceEdit("inputRSPL");
window.editor = editor;

const mode = new modeGLSL.Mode();
mode.HighlightRules = rsplHighlightRules;

editor.setTheme("ace/theme/tomorrow_night_eighties");
editor.session.setMode(mode);
editor.session.setOptions({ 
  tabSize: 2,
  useSoftTabs: true,
  newLineMode: 'unix',
  //enableBasicAutocompletion: true
});
editor.setValue(oldSource);
editor.clearSelection();

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
    console.clear();
    const source = editor.getValue();
    const parser = new Parser(Grammar.fromCompiled(grammarDef));
    const res = parser.feed(source);  
    outputError.innerHTML = "";
    writeAST(JSON.stringify(res.results, null, 2));
    outputError.innerHTML += "Code parsed without errors.\n";

    if(res.results.length > 1) {
      outputError.innerHTML += "Warning: ambiguous syntax!";
    }

    const asm = transpile(res.results[0]);
    outputError.innerHTML += "Transpiled successfully!\n";
    writeASM(asm);

    localStorage.setItem("lastCode", source);
  } catch(e) {
    outputError.innerHTML = e.message;
  }

};

//inputRSPL.oninput = () => update();
update();
editor.getSession().on('change', () => update());

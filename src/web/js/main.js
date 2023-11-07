/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

import { Parser, Grammar } from "nearley";
import grammarDef from '../../lib/grammar.cjs';

// Syntax Highlighting
import hljs from 'highlight.js/lib/core';
import mipsasm from 'highlight.js/lib/languages/mipsasm';
import json from 'highlight.js/lib/languages/json';

import rsplHighlightRules from './editorMode/rspl_highlight_rules';
import modeGLSL from 'ace-builds/src-min-noconflict/mode-glsl';

import { edit as aceEdit } from "ace-builds";
import "ace-builds/src-min-noconflict/ext-searchbox.js";
import "ace-builds/src-min-noconflict/theme-tomorrow_night_eighties";
import "ace-builds/src-min-noconflict/mode-glsl";

import { transpile } from "../../lib/transpiler";
import {EXAMPLE_CODE} from "./exampleCode.js";

// Restore old code from last session
let oldSource = localStorage.getItem("lastCode") || "";
if(oldSource === "") {
  oldSource = EXAMPLE_CODE;
}

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

let asmFileHandle = undefined;

async function saveASMFile(asmString) {
  if(!asmFileHandle) {
    const options = {
      types: [{
        description: 'MIPS ASM',
        accept: {'application/asm': ['.S', '.s'],},
      }],
    };
    asmFileHandle = await window.showSaveFilePicker(options);
  }

  if(asmString) {
    const writable = await asmFileHandle.createWritable();
    await writable.write(asmString);
    await writable.close();
  }
}

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

function setErrorState(hasError, hasWarn) {
  outputError.className = hasError ? "error" : (hasWarn ? " warn" : "");
}

async function update()
{
  try {
    console.clear();
    const source = editor.getValue();
    localStorage.setItem("lastCode", source);

    const parser = new Parser(Grammar.fromCompiled(grammarDef));

    console.time("parse");
    const res = parser.feed(source);
    console.timeEnd("parse");

    outputError.innerHTML = "";
    writeAST(JSON.stringify(res.results, null, 2));
    outputError.innerHTML += "Code parsed successfully.\n";

    if(res.results.length > 1) {
      outputError.innerHTML += "Warning: ambiguous syntax!\n";
    }

    console.time("transpile");
    const {asm, warn, info} = transpile(res.results[0]);
    console.timeEnd("transpile");
    outputError.innerHTML += info;
    outputError.innerHTML += warn + "\nTranspiled successfully!\n";
    writeASM(asm);

    if(asmFileHandle) {
      await saveASMFile(asm);
    }

    setErrorState(false, warn !== "");
  } catch(e) {
    outputError.innerHTML = e.message;
    if(!e.message.includes("Syntax error")) {
      console.error(e);
    }
    setErrorState(true, false);
  }

}

update();

let timerUpdate = 0;
editor.getSession().on('change', () => {
  clearTimeout(timerUpdate);
  timerUpdate = setTimeout(() => update(), 500);
});

copyASM.onclick = async () => {
  console.log('Copying to clipboard');
  try {
    await navigator.clipboard.writeText(outputASM.textContent);
    console.log('Content copied to clipboard');
  } catch (err) {
    console.error('Failed to copy: ', err);
  }
};

saveASM.onclick = async () => {
  await saveASMFile(outputASM.textContent);
};
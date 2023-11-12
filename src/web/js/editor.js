/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

// Ace-Editor
import {edit as aceEdit} from "ace-builds";
import rsplHighlightRules from './editorMode/rspl_highlight_rules';
import modeGLSL from 'ace-builds/src-min-noconflict/mode-glsl';

import "ace-builds/src-min-noconflict/ext-searchbox.js";
import "ace-builds/src-min-noconflict/theme-tomorrow_night_eighties";
import "ace-builds/src-min-noconflict/mode-glsl";

// Highlight.JS
import hljs from 'highlight.js/lib/core';
import mipsasm from 'highlight.js/lib/languages/mipsasm';
import json from 'highlight.js/lib/languages/json';

let highlightLines = [];
let currentAsmText = "";

hljs.registerLanguage('mipsasm', mipsasm);
hljs.registerLanguage('json', json);
hljs.highlightAll();

/**
 * Create a new RSPL editor
 * @param {string} id HTML id
 * @param {string} source
 * @return {Ace.Editor}
 */
export function createEditor(id, source)
{
    const editor = aceEdit(id);
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
    editor.setValue(source);
    editor.clearSelection();
    return editor;
}

export function codeHighlightElem(elem, newText = undefined)
{
  if(newText !== undefined)currentAsmText = newText;
  elem.textContent = currentAsmText;

  if(elem.dataset.highlighted) {
    delete elem.dataset.highlighted;
  }

  console.time("highlight");
  hljs.highlightElement(elem);
  console.timeEnd("highlight");
  codeHighlightLines(elem);
}

/**
 * @param {number[]|undefined} lines
 */
export function codeHighlightLines(elem, lines = undefined)
{
    if(lines)highlightLines = lines;

    const elemHeight = elem.parentElement.clientHeight;
    let newScroll = 11 + (15 * highlightLines[0]);
    newScroll = Math.max(0, newScroll - (elemHeight / 2));
    elem.parentElement.scrollTop = newScroll;

    console.time("highlightLine");

    if(highlightLines.length === 0) return;

    /** @type {string} */
    const code = elem.innerHTML;

    let idx = 0;
    let lineNum = 0;
    let output = "";
    while(code.indexOf("\n", idx) !== -1)
    {
        const nextIdx = code.indexOf("\n", idx);
        const line = code.substring(idx, nextIdx);
        if(highlightLines.includes(lineNum)) {
            if(!line.includes("lineMarked")) {
                output += `<span class="lineMarked">${line}</span>\n`;
            } else {
                output += line + "\n";
            }
        } else  {
            if(line.includes("lineMarked")) {
                output += line.substring(25, line.length-7) + "\n";
            } else {
                output += line + "\n";
            }
        }
        idx = nextIdx + 1;
        ++lineNum;
    }

    elem.innerHTML = output;
    console.timeEnd("highlightLine");
}
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

function getLineHeight(lineCount) {
  return 12 + (15 * lineCount);
}

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
 * @param {HTMLElement} elem
 * @param {number[]|undefined} lines
 */
export function codeHighlightLines(elem, lines = undefined)
{
    if(lines)highlightLines = lines;

    // Scroll first line into view
    const elemHeight = elem.parentElement.clientHeight;
    let newScroll = getLineHeight(highlightLines[0] || 0);
    newScroll = Math.max(0, newScroll - (elemHeight / 2));

    const oldScroll = elem.parentElement.scrollTop;
    if(Math.abs(oldScroll - newScroll) > 200) {
      elem.parentElement.scrollTo({top: newScroll, behavior: 'smooth'});
    } else {
      elem.parentElement.scrollTop = newScroll;
    }

    // Create overlays and insert into DOM
    const ovl = document.getElementById("asmOverlay");
    const newElements = [];
    for(const line of highlightLines) {
      const lineElem = document.createElement("span");
      lineElem.style.top = getLineHeight(line) + "px";
      newElements.push(lineElem);
    }
    ovl.replaceChildren(...newElements);
}
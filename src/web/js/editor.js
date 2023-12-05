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
let highlightLinesDeps = [];
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
 * @param {Record<number,number[]>|undefined} linesDeps
 */
export function codeHighlightLines(elem, lines = undefined, linesDeps = undefined)
{
    if(lines)highlightLines = lines;
    if(linesDeps)highlightLinesDeps = linesDeps;
    let hMin = Infinity, hMax = -Infinity;

    // Create overlays and insert into DOM
    const ovl = document.getElementById("asmOverlay");
    const newElements = [];

    const addLine = (height) => {
      const lineElem = document.createElement("span");
      hMin = Math.min(hMin, height);
      hMax = Math.max(hMax, height);
      lineElem.style.top = height + "px";
      newElements.push(lineElem);
      return lineElem;
    };

    let i=1;
    let posL = 50;
    let width = 6;
    for(const line of highlightLines) {
      const lineHeight = getLineHeight(line);
      addLine(lineHeight);
      const deps = highlightLinesDeps[line] || [];

      for(const dep of deps) {
        const depLineHeight = getLineHeight(dep) + 6;
        const elem = addLine(depLineHeight);
        let relHeight = (lineHeight - depLineHeight + 4);
        elem.classList.add("dep");
        elem.style.left = (posL - (i*6)) + "px";
        elem.style.width = (i*width) + "px";
        elem.style.borderColor = "hsl(" + ((i*10543) % 360) + ",40%,50%)";
        elem.style.height = relHeight + "px";
        elem.style.zIndex = 1000 - relHeight;
        ++i;
      }
    }

    ovl.replaceChildren(...newElements);

    // Scroll to midpoint of all highlighted lines
    const elemHeight = elem.parentElement.clientHeight;
    let newScroll = (hMin + hMax) / 2;
    newScroll = Math.max(0, newScroll - (elemHeight / 2));
    if(isNaN(newScroll))return;

    const oldScroll = elem.parentElement.scrollTop;
    if(Math.abs(oldScroll - newScroll) > 200) {
      elem.parentElement.scrollTo({top: newScroll, behavior: 'smooth'});
    } else {
      elem.parentElement.scrollTop = newScroll;
    }
}
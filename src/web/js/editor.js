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
let highlightLineOptDeps = [];

hljs.registerLanguage('mipsasm', mipsasm);
hljs.registerLanguage('json', json);
hljs.highlightAll();

/**
 * @param {number} i
 * @return {string}
 */
function getRandColor(i, color = 30, brightness = 50) {
  return "hsl(" + ((i*152+200) % 360) + ","+color+"%,"+brightness+"%)";
}

function getLineHeight(lineCount) {
  return 12 + (15 * lineCount);
}

/**
 * Create a new RSPL editor
 * @param {string} id HTML id
 * @param {string} source
 * @return {Ace.Editor}
 */
export function createEditor(id, source, line = 0)
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

    editor.gotoLine(line, 0, false);
    setTimeout(() => editor.scrollToLine(line, false, false, () => {}), 10);

    return editor;
}

export function codeHighlightElem(elem, newText = undefined)
{
  elem.textContent = newText;
  if(elem.dataset.highlighted) {
    delete elem.dataset.highlighted;
  }

  console.time("highlight");
  hljs.highlightElement(elem);
  console.timeEnd("highlight");
  codeHighlightLines(elem);
}

/**
 *
 * @param {HTMLElement} elem
 * @param {number} hMin
 * @param {number} hMax
 */
function scrollTo(elem, hMin, hMax)
{
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
      lineElem.style.top = height + "px";
      newElements.push(lineElem);
      return lineElem;
    };

    let posL = 32;
    let width = 8;
    let i=0;
    for(const line of highlightLines) {
      const lineHeight = getLineHeight(line);
      const elemMain = addLine(lineHeight);
      hMin = Math.min(hMin, lineHeight);
      hMax = Math.max(hMax, lineHeight);
      elemMain.style.backgroundColor = getRandColor(i, 40, 28);
      elemMain.innerText = line+"";
      const deps = (highlightLinesDeps[line] || []);

      if(deps.length > 0 && deps[0] !== deps[1])
      {
        const heightStart = getLineHeight(deps[0]) + 15;
        const heightEnd = getLineHeight(deps[1]) - 6;
        const elem = addLine(heightStart);
        let relHeight = (heightEnd - heightStart+1);

        elem.classList.add("dep");
        elem.style.left = (posL - i*width) + "px";
        elem.style.width = (i*width+4) + "px";
        elem.style.borderColor = getRandColor(i, 50);
        elem.style.height = relHeight + "px";
        elem.style.zIndex = 1000 - relHeight;
      }
      ++i;
    }

    ovl.replaceChildren(...newElements);

    // Scroll to midpoint of all highlighted lines
    scrollTo(elem, hMin, hMax);
}

/**
 * @param {HTMLElement} elem
 * @param {number[]|undefined} lines
 * @param {Record<number,number[]>|undefined} linesDeps
 */
export function codeHighlightOptLines(elem, lines = undefined, linesOptMap = undefined)
{
  if(lines)highlightLines = lines;
  if(linesOptMap)highlightLineOptDeps = linesOptMap;
  let hMin = Infinity, hMax = -Infinity;

  const ovl = document.getElementById("asmOptOverlay");
  const newElements = [];

  const addLine = (height) => {
    const lineElem = document.createElement("span");
    hMin = Math.min(hMin, height);
    hMax = Math.max(hMax, height);
    lineElem.style.top = height + "px";
    lineElem.classList.add("opt");
    newElements.push(lineElem);
    return lineElem;
  };

  let i=0;
  for(const line of highlightLines) {
    const optLine = highlightLineOptDeps[line];
    const lineHeight = getLineHeight(optLine);
    const elemMain = addLine(lineHeight);
    elemMain.style.backgroundColor = getRandColor(i, 40, 28);
    elemMain.innerText = optLine+"";
    ++i;
  }
  ovl.replaceChildren(...newElements);
  scrollTo(elem, hMin, hMax);
}
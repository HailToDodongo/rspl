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
  if(newText !== undefined)elem.textContent = newText;

  if(elem.dataset.highlighted) {
    delete elem.dataset.highlighted;
  }
  hljs.highlightElement(elem);
}
/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {transpile, transpileSource} from "../../lib/transpiler";
import {debounce} from "./utils.js";
import {codeHighlightElem, codeHighlightLines, createEditor} from "./editor.js";
import {loadSource, saveSource, saveToDevice} from "./storage.js";
import {Log} from "./logger.js";

const editor = createEditor("inputRSPL", loadSource());

/** @type {ASMOutputDebug} */
let currentDebug = {lineMap: {}};
let lastLine = 0;

function getEditorLine() {
  return editor.getCursorPosition().row + 1;
}

function highlightASM(line)
{
  if(lastLine === line) return;
  lastLine = line;

  const lines = currentDebug.lineMap[line];
  if(!lines || !lines.length) return;

  codeHighlightLines(outputASM, lines);
}

async function update()
{
  try {
    console.clear();
    const source = editor.getValue();
    saveSource(source);

    const config = {optimize: true, rspqWrapper: true};

    console.time("transpile");
    const {asm, warn, info, debug} = transpileSource(source, config);
    console.timeEnd("transpile");
    currentDebug = debug;

    Log.set(info);
    Log.append("Transpiled successfully!");

    codeHighlightElem(outputASM, asm);
    await saveToDevice("asm", asm, true);

    highlightASM(getEditorLine());

    Log.setErrorState(false, warn !== "");
  } catch(e) {
    Log.set(e.message);
    if(!e.message.includes("Syntax error")) {
      console.error(e);
    }
    Log.setErrorState(true, false);
  }
}

buttonCopyASM.onclick = async () => {
  try {
    await navigator.clipboard.writeText(outputASM.textContent);
    Log.append("Copied to clipboard!");
  } catch (err) {
    console.error('Failed to copy: ', err);
  }
};

buttonSaveASM.onclick = async () => {
  await saveToDevice("asm", outputASM.textContent);
};

update().catch(console.error);

editor.getSession().on('change', debounce(update, 150));
editor.getSession().selection.on('changeCursor', () => highlightASM(getEditorLine()));

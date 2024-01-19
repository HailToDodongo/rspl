/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {transpile, transpileSource} from "../../lib/transpiler";
import {debounce} from "./utils.js";
import {
  clearHighlightCache,
  codeHighlightElem,
  codeHighlightLines,
  codeHighlightOptLines, codeUpdateCycles, createEditor
} from "./editor.js";
import {loadLastLine, loadSource, saveLastLine, saveSource, saveToDevice} from "./storage.js";
import {Log} from "./logger.js";

/** @type {ASMOutputDebug} */
let currentDebug = {lineMap: {}, lineDepMap: {}, lineOptMap: {}, lineCycleMap: {}};
let lastLine = loadLastLine();

const editor = createEditor("inputRSPL", loadSource(), lastLine);

/** @type {RSPLConfig} */
let config = {
  optimize: true,
  rspqWrapper: true,
};

function getEditorLine() {
  return editor.getCursorPosition().row + 1;
}

function highlightASM(line)
{
  if(lastLine === line)return;
  lastLine = line;

  const lines = currentDebug.lineMap[line];
  if(!lines || !lines.length) return;

  codeHighlightLines(outputASM, lines, currentDebug.lineDepMap);
  if(Object.keys(currentDebug.lineOptMap).length > 0) {
    codeHighlightOptLines(outputASMOpt, lines, currentDebug.lineOptMap);
  } else {
    codeHighlightLines(outputASMOpt, lines, currentDebug.lineDepMap);
  }
}

async function update(reset = false)
{
  try {
    console.clear();
    if(reset) {
      clearHighlightCache();
      lastLine = 0;
    }

    const source = editor.getValue();
    saveSource(source);

    console.time("transpile");
    const {asm, asmUnoptimized, warn, info, debug} = await transpileSource(source, config);
    console.timeEnd("transpile");
    currentDebug = debug;

    Log.set(info);
    Log.append("Transpiled successfully!");

    outputASM.parentElement.parentElement.hidden = !config.optimize;
    if(config.optimize) {
      codeHighlightElem(outputASM, asmUnoptimized);
    }
    codeHighlightElem(outputASMOpt, asm);
    codeUpdateCycles(outputASMOpt, debug.lineCycleMap);

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
    await navigator.clipboard.writeText(outputASMOpt.textContent);
    Log.append("Copied to clipboard!");
  } catch (err) {
    console.error('Failed to copy: ', err);
  }
};

buttonSaveASM.onclick = async () => {
  await saveToDevice("asm", outputASMOpt.textContent);
};

optionOptimize.onchange = async () => {
  config.optimize = optionOptimize.checked;
  await update(true);
};

optionWrapper.onchange = async () => {
  config.rspqWrapper = optionWrapper.checked;
  await update(true);
};

update().catch(console.error);

editor.getSession().on('change', debounce(update, 150));
editor.getSession().selection.on('changeCursor', () => {
  const line = getEditorLine();
  highlightASM(line);
  saveLastLine(line);
});

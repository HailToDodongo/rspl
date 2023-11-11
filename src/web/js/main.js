/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {transpile, transpileSource} from "../../lib/transpiler";
import {debounce} from "./utils.js";
import {codeHighlightElem, createEditor} from "./editor.js";
import {loadSource, saveSource, saveToDevice} from "./storage.js";
import {Log} from "./logger.js";

const editor = createEditor("inputRSPL", loadSource());

async function update()
{
  try {
    console.clear();
    const source = editor.getValue();
    saveSource(source);

    const config = {optimize: true, rspqWrapper: true};

    console.time("transpile");
    const {asm, warn, info} = transpileSource(source, config);
    console.timeEnd("transpile");

    Log.set(info);
    Log.append("Transpiled successfully!");

    codeHighlightElem(outputASM, asm);
    await saveToDevice("asm", asm, true);

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

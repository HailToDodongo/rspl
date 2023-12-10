/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {EXAMPLE_CODE} from "./exampleCode.js";

const STORAGE_KEY = "lastCode03";
const FILE_HANDLES = {};

export function loadSource() {
  let oldSource = localStorage.getItem(STORAGE_KEY) || "";
  return (oldSource === "") ? EXAMPLE_CODE : oldSource;
}

export function saveSource(source) {
  localStorage.setItem(STORAGE_KEY, source);
}

export function saveLastLine(line) {
  localStorage.setItem(STORAGE_KEY + "_line", line);
}

export function loadLastLine() {
  return localStorage.getItem(STORAGE_KEY + "_line") || 0;
}

export async function saveToDevice(id, data, skipDialog = false)
{
  if(!FILE_HANDLES[id]) {
    if(skipDialog)return;

    const options = {
      types: [{
        description: 'MIPS ASM',
        accept: {'application/asm': ['.S', '.s'],},
      }],
    };
    FILE_HANDLES[id] = await window.showSaveFilePicker(options);
  }

  if(data) {
    const writable = await FILE_HANDLES[id].createWritable();
    await writable.write(data);
    await writable.close();
  }
}
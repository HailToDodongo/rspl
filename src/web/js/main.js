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
import "ace-builds/src-min-noconflict/theme-tomorrow_night_eighties";
import "ace-builds/src-min-noconflict/mode-glsl";

import { transpile } from "../../lib/transpiler";

// Restore old code from last session
let oldSource = localStorage.getItem("lastCode") || "";
if(oldSource === "") {
  oldSource = `include "rsp_queue.inc"

state
{ 
  vec32 VEC_SLOTS[20];
  u32 TEST_CONST;
}

function test_scope() 
{
  u32<$t0> a;
  {
     u32<$t1> b;
     b += 2;
  } // 'b' is no longer defined now
  a += 2;
}

function test_vector_load() 
{
  vec32<$t0> src;
  vec32<$v01> dst;
  // Whole Vector
  dst = load(src);
  dst = load(src, 0x10);
  dst.y = load(src);
  dst.z = load(src, 0x10);
  //dst = load(src, TEST_CONST); Invalid
  //dst = load(TEST_CONST); Invalid
  //dst = load(TEST_CONST, 0x10); Invalid
  
  // Swizzle
  dst = load(src).xyzwxyzw;
  dst = load(src, 0x10).xyzwxyzw;
  dst.y = load(src).xyzwxyzw;
  dst.z = load(src, 0x10).xyzwxyzw;
  //dst = load(src, TEST_CONST).xyzwxyzw; Invalid
  //dst = load(TEST_CONST).xyzwxyzw; Invalid
  //dst = load(TEST_CONST, 0x10).xyzwxyzw; Invalid
}

function test_label()
{
  u32<$t0> a;
  
  label_a:
    a += 2;
    goto label_b;
    a += 0xFF;
    
  label_b:
    goto label_a;
}

function test_scalar_load()
{
  u32<$t0> src, dst;
  // destination = load(source, offset)
  dst = load(src);
  dst = load(src, 0x10);
  dst = load(src, TEST_CONST);
  
  dst = load(TEST_CONST);
  dst = load(TEST_CONST, 0x10);
  // dst = load(TEST_CONST, TEST_CONST); Invalid
}

function test_scalar_ops()
{
  u32<$t0> a, b, c;
  s32<$t3> sa, sb, sc;
  
  // Add
  c = a + b; sc = sa + sb;
  c = a + 1; sc = sa + 1;
  c = a + TEST_CONST; sc = sa + TEST_CONST;
  // Sub
  c = a - b; sc = sa - sb;
  c = a - 1; sc = sa - 1;
  //c = a - TEST_CONST; sc = sa - TEST_CONST;
  // Mul/Div not possible
  // And
  c = a & b;
  c = a & 1;
  c = a & TEST_CONST;
  // Or
  c = a | b;
  c = a | 2;
  c = a | TEST_CONST;
  // XOR
  c = a ^ b;
  c = a ^ 2;
  c = a ^ TEST_CONST;
  // Not
  c = ~b;
  // Shift-Left
  c = a << b;
  c = a << 2;
  //c = a << TEST_CONST;
  // Shift-Right
  c = a >> b;
  c = a >> 2;
  //c = a >> TEST_CONST; 
}

command<0> VecCmd_Transform(u32 vec_out, u32 mat_in)
{
  u32<$t0> trans_mtx = mat_in >> 16;
  trans_mtx &= 0xFF0;

  u32<$t1> trans_vec = mat_in & 0xFF0;
  u32<$t2> trans_out = vec_out & 0xFF0;
  
  trans_mtx += VEC_SLOTS;
  trans_vec += VEC_SLOTS;
  trans_out += VEC_SLOTS;
  
  vec32<$v01> mat0 = load(trans_mtx, 0x00).xyzwxyzw;
  vec32<$v03> mat1 = load(trans_mtx, 0x08).xyzwxyzw;
  vec32<$v05> mat2 = load(trans_mtx, 0x20).xyzwxyzw;
  vec32<$v07> mat3 = load(trans_mtx, 0x28).xyzwxyzw;
  
  vec32<$v09> vecIn = load(trans_vec);
  vec32<$v13> res;
  
  res = mat0  * vecIn.xxxxXXXX;
  res = mat1 +* vecIn.yyyyYYYY;
  res = mat2 +* vecIn.zzzzZZZZ;
  res = mat3 +* vecIn.wwwwWWWW;

  trans_out = store(res);
}`;
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

function update()
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
    const asm = transpile(res.results[0]);
    console.timeEnd("transpile");
    outputError.innerHTML += "Transpiled successfully!\n";
    writeASM(asm);
  } catch(e) {
    outputError.innerHTML = e.message;
    if(!e.message.includes("Syntax error")) {
      console.error(e);
    }
  }

};

//inputRSPL.oninput = () => update();
update();

let timerUpdate = 0;
editor.getSession().on('change', () => {
  clearTimeout(timerUpdate);
  timerUpdate = setTimeout(() => update(), 100);
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

#!/usr/bin/env node
// Convert an ares rspTrace instruction sequence + the asm source (.inc) into
// bracket-annotated test snippets for rspl's test_evalCostExample.cpp.
//
// usage: node trace2test.js <seqFile> <incFile> <entryLabel> <basePC-hex> [--skip N] [--drop-last N]
//
// seqFile: lines "RSP  <PC>  [ <cyc>]  ... | <op> ..." (grep '^RSP  ' from trace)
// The traced dynamic path is split into linear segments at PC discontinuities
// (taken branches / entry jump). Each segment is emitted as its own snippet,
// cycles rebased to the segment start. All branches are marked "# unlikely"
// (the model's taken-branch bubble falls on the segment seam, which rebasing
// absorbs — segment-internal branches on the traced path were not taken).

const fs = require("fs");
const [seqFile, incFile, entryLabel, basePcHex, ...rest] = process.argv.slice(2);
const basePC = parseInt(basePcHex, 16);
let skip = 0, dropLast = 0;
for (let i = 0; i < rest.length; i++) {
  if (rest[i] === "--skip") skip = +rest[++i];
  if (rest[i] === "--drop-last") dropLast = +rest[++i];
}

// ---- parse .inc: ordered instruction list from entryLabel onward ----
const incLines = fs.readFileSync(incFile, "utf8").split("\n");
let started = false;
const instrs = []; // {text}
for (let raw of incLines) {
  let line = raw;
  const cut = line.indexOf("##");
  if (cut >= 0) line = line.slice(0, cut);
  // strip //-comments too (ref file)
  const cut2 = line.indexOf("//");
  if (cut2 >= 0) line = line.slice(0, cut2);
  line = line.trim();
  if (!started) {
    if (line === entryLabel + ":") started = true;
    continue;
  }
  if (!line) continue;
  if (/^[A-Za-z_$.][\w.$]*:$/.test(line)) continue;       // label
  line = line.replace(/^[A-Za-z_$.][\w.$]*:\s*/, "");     // inline label prefix
  if (!line) continue;
  if (line.startsWith(".") || line.startsWith("#")) continue; // directive / cpp
  // normalize whitespace + comma spacing
  line = line.replace(/\s*,\s*/g, ", ").replace(/\s+/g, " ");
  instrs.push(line);
}

// ---- parse trace sequence ----
const seq = [];
for (const l of fs.readFileSync(seqFile, "utf8").split("\n")) {
  const m = l.match(/^RSP\s+([0-9A-F]+)\s+\[\s*(\d+)\]\s*\S*\s*\|\s*(\S+)/);
  if (!m) continue;
  seq.push({ pc: parseInt(m[1], 16), cyc: +m[2], op: m[3].toLowerCase() });
}
const body = seq.slice(skip, seq.length - dropLast);

// ares-disasm mnemonic -> rspl mnemonic aliases (either direction ok)
const alias = {
  liu: ["ori", "lui", "addiu"], subiu: ["addiu"], add: ["addu"], sub: ["subu"],
  move: ["or", "addu"], "nop": ["nop"],
};
const BRANCH = new Set(["beq","bne","blez","bgtz","bltz","bgez","bltzal","bgezal","j","jal","jr","jalr","b","bal"]);

function opOf(text) { return text.split(/\s+/)[0].toLowerCase(); }

// ---- split into segments & emit ----
const segs = [];
let cur = null;
let prevPc = null;
for (const e of body) {
  if (prevPc === null || e.pc !== prevPc + 4) { cur = []; segs.push(cur); }
  cur.push(e);
  prevPc = e.pc;
}

let mismatches = 0;
const out = [];
segs.forEach((seg, si) => {
  const lines = [];
  const base = seg[0].cyc;
  for (const e of seg) {
    const idx = (e.pc - basePC) / 4;
    if (idx < 0 || idx >= instrs.length || !Number.isInteger(idx)) {
      console.error(`PC ${e.pc.toString(16)} out of .inc range (idx ${idx})`);
      process.exit(1);
    }
    let text = instrs[idx];
    const srcOp = opOf(text);
    if (srcOp !== e.op && !(alias[e.op] || []).includes(srcOp)) {
      console.error(`MISMATCH pc=${e.pc.toString(16)} trace='${e.op}' inc='${text}'`);
      mismatches++;
    }
    if (BRANCH.has(srcOp)) text += "  # unlikely";
    lines.push(`[${String(e.cyc - base).padStart(3)}] ${text}`);
  }
  out.push({ si, first: seg[0].pc, lines, cycles: seg[seg.length - 1].cyc - base + 1 });
});

if (mismatches) { console.error(`${mismatches} mnemonic mismatches — aborting`); process.exit(1); }

for (const s of out) {
  console.log(`// --- segment ${s.si}: entry pc 0x${s.first.toString(16)}, ${s.lines.length} instr, ${s.cycles} cycles ---`);
  for (const l of s.lines) console.log(l);
  console.log("");
}
console.error(`OK: ${out.length} segments, ${body.length} instructions total`);

#!/usr/bin/env node
// Reconstruct rspl-syntax instruction text from ares rspTrace disassembly and
// emit bracket-annotated eval_cost test snippets (same output format as
// trace2test.js). Optionally cross-check against a generated .inc.
//
// usage: node disasm2test.js <seqFile> [--skip N] [--drop-last N]
//                            [--check <incFile> <entryLabel> <basePC-hex>]

const fs = require("fs");
const argv = process.argv.slice(2);
const seqFile = argv[0];
let skip = 0, dropLast = 0, check = null;
for (let i = 1; i < argv.length; i++) {
  if (argv[i] === "--skip") skip = +argv[++i];
  if (argv[i] === "--drop-last") dropLast = +argv[++i];
  if (argv[i] === "--check") check = { inc: argv[++i], label: argv[++i], base: parseInt(argv[++i], 16) };
}

const SCALAR = new Set(["zero","at","v0","v1","a0","a1","a2","a3","t0","t1","t2","t3","t4","t5","t6","t7","s0","s1","s2","s3","s4","s5","s6","s7","t8","t9","k0","k1","gp","sp","fp","s8","ra"]);
const COP0 = {
  SP_PBUS_ADDRESS:"COP0_DMA_SPADDR", SP_DRAM_ADDRESS:"COP0_DMA_RAMADDR",
  SP_READ_LENGTH:"COP0_DMA_READ", SP_WRITE_LENGTH:"COP0_DMA_WRITE",
  SP_DMA_BUSY:"COP0_DMA_BUSY", SP_DMA_FULL:"COP0_DMA_FULL",
  SP_STATUS:"COP0_SP_STATUS", SP_SEMAPHORE:"COP0_SEMAPHORE",
  DPC_START:"COP0_DP_START", DPC_END:"COP0_DP_END", DPC_CURRENT:"COP0_DP_CURRENT",
  DPC_STATUS:"COP0_DP_STATUS",
};
function sreg(t) { // scalar register (ares prints "0" for $zero)
  if (t === "0") return "$zero";
  if (!SCALAR.has(t)) throw new Error("not a scalar reg: " + t);
  return "$" + t;
}
function vreg(t) { // "v9" -> "$v09", optional [e]
  const m = t.match(/^v(\d+)(\[(\d+)\])?$/);
  if (!m) throw new Error("not a vector reg: " + t);
  return { r: "$v" + m[1].padStart(2, "0"), e: m[3] !== undefined ? +m[3] : null };
}
function swz(e) { // computational element index -> rspl suffix
  if (e === null || e === 0 || e === 1) return "";
  if (e === 2 || e === 3) return ".q" + (e - 2);
  if (e <= 7) return ".h" + (e - 4);
  return ".e" + (e - 8);
}
function imm(t) { // "$FF" -> "0xFF", decimal stays
  if (t.startsWith("$")) return "0x" + t.slice(1);
  return t;
}
function mem(t) { // "a0+$6" -> {off, base}; "0+$3F8" -> base $zero
  const m = t.match(/^(\w+)\+\$([0-9A-F]+)$/i);
  if (!m) throw new Error("bad mem operand: " + t);
  return { off: parseInt(m[2], 16), base: sreg(m[1]) };
}

const R3 = new Set(["add","addu","sub","subu","or","xor","and","nor","slt","sltu"]);
const RI = new Set(["addiu","andi","ori","xori","slti","sltiu"]);
const SHIFT = new Set(["sll","srl","sra"]);
const LSU = new Set(["lb","lbu","lh","lhu","lw","lwu","sb","sh","sw"]);
const VLS = new Set(["lqv","lrv","ldv","llv","lsv","lbv","luv","lpv","lhv","lfv","ltv","sqv","srv","sdv","slv","ssv","sbv","suv","spv","shv","sfv","stv","swv"]);
const VOP3 = new Set(["vmulf","vmulu","vmacf","vmacu","vmudl","vmudm","vmudn","vmudh","vmadl","vmadm","vmadn","vmadh","vadd","vsub","vaddc","vsubc","vabs","vand","vnand","vor","vnor","vxor","vnxor","vlt","veq","vne","vge","vcl","vch","vcr","vmrg"]);
const VELEM2 = new Set(["vrcp","vrcpl","vrcph","vrsq","vrsql","vrsqh","vmov"]);
const BR2 = new Set(["beq","bne"]);
const BR1 = new Set(["blez","bgtz","bltz","bgez","bltzal","bgezal"]);
const ACC = { acch:"COP2_ACC_HI", accm:"COP2_ACC_MD", accl:"COP2_ACC_LO" };
const CTRL = { vcc:"$vcc", vco:"$vco", vce:"$vce" };

function convert(op, argStr) {
  const a = argStr ? argStr.split(",").map(s => s.trim()).filter(s => s.length) : [];
  if (op === "nop") return "nop";
  if (op === "liu") return `addiu ${sreg(a[0])}, $zero, ${imm(a[1])}`;
  if (op === "subiu") return `addiu ${sreg(a[0])}, ${sreg(a[1])}, -${imm(a[2]).replace(/^0x/, "0x")}`;
  if (op === "lui") return `lui ${sreg(a[0])}, ${imm(a[1])}`;
  if (R3.has(op)) return `${op} ${sreg(a[0])}, ${sreg(a[1])}, ${sreg(a[2])}`;
  if (RI.has(op)) return `${op} ${sreg(a[0])}, ${sreg(a[1])}, ${imm(a[2])}`;
  if (SHIFT.has(op)) return `${op} ${sreg(a[0])}, ${sreg(a[1])}, ${a[2]}`;
  if (LSU.has(op)) { const m = mem(a[1]); return `${op} ${sreg(a[0])}, ${m.off}(${m.base})`; }
  if (VLS.has(op)) { const v = vreg(a[0]); const m = mem(a[1]); return `${op} ${v.r}, ${v.e ?? 0}, ${m.off}, ${m.base}`; }
  if (op === "mtc2" || op === "mfc2") { const v = vreg(a[1]); return `${op} ${sreg(a[0])}, ${v.r}.e${(v.e ?? 0) / 2}`; }
  if (op === "cfc2" || op === "ctc2") return `${op} ${sreg(a[0])}, ${CTRL[a[1]] || a[1]}`;
  if (op === "mfc0") return `mfc0 ${sreg(a[0])}, ${COP0[a[1]] || "COP0_" + a[1]}`;
  if (op === "mtc0") return `mtc0 ${sreg(a[1])}, ${COP0[a[0]] || "COP0_" + a[0]}`;
  if (op === "vsar") { const v = vreg(a[0]); return `vsar ${v.r}, ${ACC[a[2]] || a[2]}`; }
  if (VOP3.has(op)) {
    const d = vreg(a[0]), s = vreg(a[1]), t = vreg(a[2]);
    return `${op} ${d.r}, ${s.r}, ${t.r}${swz(t.e)}`;
  }
  if (VELEM2.has(op)) {
    const d = vreg(a[0]), s = vreg(a[1]);
    return `${op} ${d.r}.e${(d.e ?? 8) - 8}, ${s.r}.e${(s.e ?? 8) - 8}`;
  }
  if (BR2.has(op)) return `${op} ${sreg(a[0])}, ${sreg(a[1])}, LABEL_${a[2].replace("$", "")}`;
  if (BR1.has(op)) return `${op} ${sreg(a[0])}, LABEL_${a[1].replace("$", "")}`;
  if (op === "jr" || op === "jalr") return `${op} ${sreg(a[0])}`;
  if (op === "j" || op === "jal") return `${op} LABEL_${a[0].replace("$", "")}`;
  throw new Error("unhandled op: " + op + " " + argStr);
}

const BRANCH = new Set([...BR2, ...BR1, "j", "jal", "jr", "jalr"]);

// ---- parse trace ----
const seq = [];
for (const l of fs.readFileSync(seqFile, "utf8").split("\n")) {
  const m = l.match(/^RSP\s+([0-9A-F]+)\s+\[\s*(\d+)\]\s*\S*\s*\|\s*(\S+)\s*(.*)$/);
  if (!m) continue;
  const args = m[4].replace(/\{[^}]*\}/g, "").trim();
  seq.push({ pc: parseInt(m[1], 16), cyc: +m[2], op: m[3].toLowerCase(), args });
}
const body = seq.slice(skip, seq.length - dropLast);

// ---- optional cross-check against .inc-derived text ----
if (check) {
  const incLines = fs.readFileSync(check.inc, "utf8").split("\n");
  let started = false; const instrs = [];
  for (let raw of incLines) {
    let line = raw;
    for (const c of ["##", "//"]) { const i = line.indexOf(c); if (i >= 0) line = line.slice(0, i); }
    line = line.trim();
    if (!started) { if (line === check.label + ":") started = true; continue; }
    if (!line) continue;
    if (/^[A-Za-z_$.][\w.$]*:$/.test(line)) continue;
    line = line.replace(/^[A-Za-z_$.][\w.$]*:\s*/, "");
    if (!line || line.startsWith(".") || line.startsWith("#")) continue;
    instrs.push(line.replace(/\s*,\s*/g, ", ").replace(/\s+/g, " "));
  }
  // signature: op + register/element tokens (order kept), ignore imms/addresses
  const sig = t => {
    const op = t.split(/\s+/)[0].toLowerCase();
    const regs = [...t.matchAll(/\$(?:v\d+(?:\.[a-z0-9]+)?|[a-z]+\d*)|COP[02]_[A-Z_]+|\$vcc|\$vco/gi)]
      .map(m => m[0].toLowerCase().replace(/\.v$/, "")).filter(r => r !== "$zero");
    return op + " " + regs.join(" ");
  };
  let bad = 0;
  for (const e of body) {
    const idx = (e.pc - check.base) / 4;
    const mine = convert(e.op, e.args);
    const ref = instrs[idx];
    if (sig(mine) !== sig(ref)) {
      console.error(`DIFF pc=${e.pc.toString(16)}\n  disasm: ${mine}\n  inc:    ${ref}`);
      bad++;
    }
  }
  console.error(bad ? `${bad} signature diffs` : `CHECK OK: all ${body.length} instructions match .inc signatures`);
  process.exit(bad ? 1 : 0);
}

// ---- emit segments ----
const segs = [];
let cur = null, prevPc = null;
for (const e of body) {
  if (prevPc === null || e.pc !== prevPc + 4) { cur = []; segs.push(cur); }
  cur.push(e);
  prevPc = e.pc;
}
segs.forEach((seg, si) => {
  const base = seg[0].cyc;
  console.log(`// --- segment ${si}: entry pc 0x${seg[0].pc.toString(16)}, ${seg.length} instr, ${seg[seg.length - 1].cyc - base + 1} cycles ---`);
  for (const e of seg) {
    let text = convert(e.op, e.args);
    if (BRANCH.has(e.op)) text += "  # unlikely";
    console.log(`[${String(e.cyc - base).padStart(3)}] ${text}`);
  }
  console.log("");
});
console.error(`OK: ${segs.length} segments, ${body.length} instructions`);

import {evalFunctionCost} from "../../../lib/optimizer/eval/evalCost.js";
import {asm, asmLabel, asmNOP} from "../../../lib/intsructions/asmWriter.js";
import {asmInitDeps, asmScanDeps} from "../../../lib/optimizer/asmScanDeps.js";

function textToAsmLines(text)
{
  const lines = text.trim().split("\n");
  return lines.map(line => {
    const [op, ...args] = line.trim().split(" ");
    args.forEach((arg, i) => {
      if(arg.endsWith(","))args[i] = arg.slice(0, -1);
    });
    return asm(op, args);
  });
}

/**
 * @param {ASM[]} lines
 */
function linesToCycles(lines)
{
  const func = {asm: lines};
  asmInitDeps(func);
  evalFunctionCost(func);
  return lines.map(line => line.debug.cycle);
}

describe('Eval - Cost', () =>
{
  test('SU only - no dep', async () => {
    const lines = [
      asm("or", ["$t0", "$zero", "$zero"]),
      asm("addiu", ["$t0", "$t0", "1"]),
      asm("addiu", ["$t0", "$t0", "1"]),
      asm("addiu", ["$t0", "$t0", "1"]),
      asm("jr", ["$ra"]),
      asmNOP(),
    ];
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3,4,5,6
    ]);
  });

  test('SU only - deps', async () => {
    const lines = textToAsmLines(`
      or $t0, $zero, $zero
      or $t1, $zero, $t0
      addu $t2, $t0, $t1
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3
    ]);
  });

  test('VU only - no dep', async () => {
    const lines = textToAsmLines(`
      vxor $v01, $v00, $v00.e0
      vxor $v01, $v00, $v30.e7
      vxor $v01, $v00, $v30.e6
      vxor $v01, $v00, $v30.e5
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3,4
    ]);
  });

  test('VU only - ACC / 32-bit mul', async () => {
    const lines = textToAsmLines(`
      vmudl $v29, $v05, $v09.v
      vmadm $v29, $v04, $v09.v
      vmadn $v11, $v05, $v08.v
      vmadh $v10, $v04, $v08.v
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3,4
    ]);
  });

  test('VU only - DIV / invert_half', async () => {
    const lines = textToAsmLines(`
      vrcph $v04.e0, $v04.e0
      vrcpl $v05.e0, $v05.e0
      vrcph $v04.e0, $v08.e0
      vrcpl $v09.e0, $v09.e0
      vrcph $v08.e0, $v00.e0
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3,4,5
    ]);
  });

  test('VU only - ternary', async () => {
    const lines = textToAsmLines(`
      vaddc $v06, $v06, $v06.v
      vadd $v11, $v05, $v05.v
      vne $v29, $v18, $v00.e0
      vmrg $v13, $v05, $v07
      vmrg $v14, $v06, $v08
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3,4,5
    ]);
  });

  test('VU only - deps', async () => {
    const lines = textToAsmLines(`
      vxor $v01, $v00, $v00.e0
      vaddc $v04, $v01, $v01.v
      vxor $v05, $v00, $v00.e0
      vaddc $v04, $v01, $v01.v
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,5,6,7
    ]);
  });

  test('SU/VU mix - no dep', async () => {
    const lines = textToAsmLines(`
      vxor $v01, $v00, $v00.e0
      vxor $v01, $v00, $v30.e7
      addiu $t0, $zero, 4
      vxor $v01, $v00, $v30.e6
      addiu $t1, $zero, 4
      addiu $t2, $zero, 4
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1, 2, 2, // vxor + addiu
      3, 3,    // vxor + addiu
      4
    ]);
  });

  test('SU/VU mix - same src/dst dep', async () => {
    const lines = textToAsmLines(`
      vxor $v01, $v00, $v30.e7
      vxor $v01, $v01, $v01
      vor $v05, $v00, $v01
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,5,9
    ]);
  });

  test('SU/VU mix - delay slot', async () => {
    const lines = textToAsmLines(`
      or $t0, $zero, $zero
      bne $t0, $zero, END
      nop
      vxor $v01, $v00, $v30.e7
      vaddc $v02, $v03, $v30.e7
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3,4,5
    ]);
  });

  test('SU/VU mix - MTC2', async () => {
    const lines = textToAsmLines(`
      vxor $v04, $v00, $v00.e0
      vxor $v05, $v00, $v00
      mtc2 $t0, $v05.e0
      srl $at, $t0, 16
      srl $at, $t0, 16
      mtc2 $at, $v04.e0
      vmov $v04.e2, $v04.e0
      vmov $v05.e2, $v05.e0
      vmov $v04.e3, $v04.e1
      vmov $v05.e3, $v05.e1
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1, 2, 3, 4, 5, 6,
      10, 11, 14, 15
    ]);
  });

  test('SU memory - load/store (dep)', async () => {
    const lines = textToAsmLines(`
      lw $t1, 0($t0)
      addiu $t2, $zero, 3
      sw $t1, ($t0)
      addiu $t2, $zero, 4
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,4,5
    ]);
  });

  test('SU memory - load/store (no-dep)', async () => {
    const lines = textToAsmLines(`
      lw $t4, 0($t0)
      addiu $t2, $zero, 3
      sw $t1, ($t0)
      addiu $t2, $zero, 4
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,4,5
    ]);
  });
  
    test('SU memory - load/store (no-dep, dual)', async () => {
    const lines = textToAsmLines(`
      lw $t4, 0($t0)
      vxor $v11, $v11, $v11
      addiu $t2, $zero, 3
      sw $t1, ($t0)
      addiu $t2, $zero, 4
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,1,2,4,5
    ]);
  });

// @TODO:
  // branches
  // delay-dual issue
  // hardware-bug (VRCP, VRCPL, VRCPH, VMOV, VRSQ, VRSQL, VRSQH, VNOP)
});
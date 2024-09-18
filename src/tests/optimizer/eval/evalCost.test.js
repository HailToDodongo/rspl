import {evalFunctionCost} from "../../../lib/optimizer/eval/evalCost.js";
import {asm, asmLabel, asmNOP} from "../../../lib/intsructions/asmWriter.js";
import {asmInitDeps, asmScanDeps} from "../../../lib/optimizer/asmScanDeps.js";

function textToAsmLines(text)
{
  const lines = text.trim().split("\n");
  return lines
      .map(line => line.trim())
      .filter(line => line.length > 0)
      .map(line => {
        const [op, ...args] = line.trim().split(" ");
        args.forEach((arg, i) => {
          if(arg.endsWith(","))args[i] = arg.slice(0, -1);
        });
        return op === "nop" ? asmNOP() : asm(op, args);
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
    const lines = textToAsmLines(`
      or    $t0, $zero, $zero
      addiu $t0, $t0, 1
      addiu $t0, $t0, 1
      addiu $t0, $t0, 1
      jr    $ra
      nop
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3,4,5,7
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

  test('VU - same src/dst dep', async () => {
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

  test('VU/SU - no dual issue', async () => {
    const lines = textToAsmLines(`
      vand $v04, $v30, $v31
      lqv $v04, 0, 0, $s4
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2
    ]);
  });

  test('VU/SU - no dual issue', async () => {
    const lines = textToAsmLines(`
      vand $v04, $v30, $v31
      mfc2 $t0, $v04.e4 
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,5
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
      1,2,4,5,6
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
      1, 2,
      3, 4, 5, 6,
      10, 11, 14, 15
    ]);
  });

  test('SU/VU in dual - MFC2', async () => {
    const lines = textToAsmLines(`      
      vmadn $v06, $v06, $v05.h3
      vmadh $v05, $v05, $v05.h3
      nop   
      mfc2 $fp, $v02.e2   
      vmudl $v29, $v06, $v14.v 
      vor $v00, $v00, $v00
      nop   
      sra $fp, $fp, 7   
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,2,3,3+2,6,6,8
    ]);
  });

  // @TODO: test VU+SU, SU would be fine but VU stalls, then SU is in a stall because of 2 cycle load/store bs

  test('SU memory - load (dep)', async () => {
    const lines = textToAsmLines(`
      lhu $s3, 24($s4)
      lhu $s2, 24($s4)
      srl $s2, $s2, 2
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,5
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

  test('SU memory - load/store (multiple)', async () => {
    const lines = textToAsmLines(`
      lhu $s3, 24($s4)
      lhu $s2, 24($s4)
      lhu $s1, 24($s4)
      sh $s3, 12($s6)
      sh $s2, 12($s5)
      sh $s1, 12($s5)
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,3,
      6,7,8
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

  test('CFC2 - stall', async () => {
    const lines = textToAsmLines(`
      cfc2 $sp, $vcc
      andi $sp, $sp, 1799
      srl $t7, $sp, 5
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1, 2+2, 5
    ]);
  });

  test('VU + CFC2 - dual', async () => {
    const lines = textToAsmLines(`
      vxor $v01, $v01, $v01
      cfc2 $sp, $vcc
      andi $sp, $sp, 1799
      srl $t7, $sp, 5
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1, 1, 2+2, 5
    ]);
  });

  test('CFC2 + VU - dual', async () => {
    const lines = textToAsmLines(`
      cfc2 $sp, $vcc
      vxor $v01, $v01, $v01
      andi $sp, $sp, 1799
      srl $t7, $sp, 5
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1, 1, 2+2, 5
    ]);
  });

  test('VU + CFC2 - no-dual', async () => {
    const lines = textToAsmLines(`
      vcl $v29, $v27, $v20
      cfc2 $sp, $vcc
      andi $sp, $sp, 1799
      srl $t7, $sp, 5
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2, 3+2, 6
    ]);
  });

  test('CFC2 + VU - dual', async () => {
    const lines = textToAsmLines(`
      cfc2 $sp, $vcc
      vcl $v29, $v27, $v20
      andi $sp, $sp, 1799
      srl $t7, $sp, 5
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,1, 2+2, 5
    ]);
  });

  test('Branch - NOP', async () => {
    const lines = textToAsmLines(`
      or $s7, $zero, $zero
      beq $s7, $zero, LABEL_0001
      nop
      vxor $v28, $v00, $v30.e7
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,4,5
    ]);
  });

  test('Branch multiple - NOP', async () => {
    const lines = textToAsmLines(`
      beq $zero, $zero, LABEL_A
      nop
      beq $zero, $zero, LABEL_B
      nop
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1, 2+1,
      4, 5+1
    ]);
  });

  test('Branch - filled (scalar)', async () => {
    const lines = textToAsmLines(`
      or $s7, $zero, $zero
      beq $s7, $zero, LABEL_0001
      addiu $s6, $zero, 3
      addiu $s6, $zero, 1
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,4,5
    ]);
  });

  test('Branch - filled + stall (scalar)', async () => {
    const lines = textToAsmLines(`
      lw $a0, %lo(SCREEN_SIZE_VEC + 0)
      beq $zero, $zero, LABEL_A
      addiu $a0, $a0, 1
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,5
    ]);
  });

  test('Branch - filled (vector)', async () => {
    const lines = textToAsmLines(`
      or $s7, $zero, $zero
      beq $s7, $zero, LABEL_0001
      vxor $v28, $v00, $v30.e7
      vxor $v28, $v00, $v30.e7
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1,2,4,5
    ]);
  });

  test('Example - A', async () => {
    const lines = textToAsmLines(`
      or $t2, $zero, $zero
      andi $t0, $a0, 65535
      ori $s4, $zero, %lo(TRI_BUFFER)
      or $s0, $zero, $a1
      addiu $s4, $s4, 1376
      addu $s7, $s4, $t0
      jal DMAExec
      addiu $t0, $t0, -1
      ori $s6, $zero, %lo(MAT_MODEL_RROJ)
      ldv $v21, 0, 48, $s6
      ldv $v22, 0, 56, $s6
      ldv $v25, 0, 16, $s6
      ldv $v21, 8, 48, $s6
      ldv $v25, 8, 16, $s6
      ldv $v24, 0, 40, $s6
      ldv $v22, 8, 56, $s6
      ldv $v26, 0, 24, $s6
      ori $at, $zero, %lo(COLOR_AMBIENT)
      ldv $v26, 8, 24, $s6
      luv $v14, 0, 0, $at
      ldv $v23, 0, 32, $s6
      ldv $v24, 8, 40, $s6
      ldv $v23, 8, 32, $s6
      ori $at, $zero, %lo(NORMAL_MASK_SHIFT)
      ldv $v27, 0, 0, $s6
      ldv $v27, 8, 0, $s6
      ldv $v28, 0, 8, $s6
      ldv $v28, 8, 8, $s6
      ori $s6, $zero, %lo(MAT_MODEL_NORM)
      ldv $v13, 0, 0, $at
      ldv $v20, 0, 8, $s6
      ldv $v12, 0, 8, $at
      ldv $v13, 8, 0, $at
      ldv $v17, 0, 16, $s6
      ldv $v12, 8, 8, $at
      ldv $v19, 0, 0, $s6
      ldv $v20, 8, 8, $s6
      ori $at, $zero, %lo(CLIPING_PLANES)
      ldv $v15, 0, 32, $s6
      ldv $v18, 0, 24, $s6
      llv $v11, 0, 0, $at
      ldv $v18, 8, 24, $s6
      ldv $v19, 8, 0, $s6
      ori $at, $zero, %lo(SCREEN_SIZE_VEC)
      ldv $v17, 8, 16, $s6
      ldv $v16, 0, 40, $s6
      ldv $v15, 8, 32, $s6
      ldv $v10, 0, 0, $at
      ldv $v09, 0, 8, $at
      vor $v11, $v00, $v11.e1
      ldv $v10, 8, 0, $at
      ldv $v16, 8, 40, $s6
      ldv $v09, 8, 8, $at
      ori $s6, $zero, %lo(TRI_BUFFER)
      jal DMAWaitIdle
      addiu $s5, $s6, 38
      `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
      1, 2, 3, 4, 5, 6, 7,
      9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
      25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
      40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
      51, 52, 53, 54, 55, 57
    ]);
  });

  test('Example - B', async () => {
    const lines = textToAsmLines(`
      vmov $v15.e7, $v23.e3
      vmudl $v29, $v15, $v17.v
      ori $at, $zero, 0xFFFF
      vmadm $v29, $v14, $v17.v
      vmadn $v13, $v15, $v16.v
      vmadh $v12, $v14, $v16.v
      vaddc $v13, $v13, $v13.q1
      vadd $v12, $v12, $v12.q1
      vaddc $v13, $v13, $v13.h2
      vadd $v12, $v12, $v12.h2
      sqv $v25, 0, 16, $s0
      vsubc $v11, $v13, $v13.e4
      vsub $v10, $v12, $v12.e4
      vrcph $v10.e0, $v10.e0
      vrcpl $v11.e0, $v11.e0
      vrcph $v10.e0, $v00.e0
      vmudl $v29, $v11, $v13.e0
      vmadm $v29, $v10, $v13.e0
      vmadn $v11, $v11, $v12.e0
      vmadh $v10, $v10, $v12.e0
      vaddc $v11, $v11, $v11.v
      mtc2 $at, $v11.e1
      vadd $v10, $v10, $v10.v
      `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
    1,
    2+3,  5,
    6,
    7,
    8,
    9+2,
    12,
    13+2,
    16,
    16,
    17+2,
    20,
    21+3,
    25,
    26,
    27+2,
    30,
    31,
    32,
    33+2,
    36, 36]);
  });

  test('Example - C', async () => {
    const lines = textToAsmLines(`
      vaddc $v28, $v28, $v20.v
        slv $v26, 8, 20, $s0
      sdv $v27, 0, 8, $s0
        vor $v20, $v00, $v27.e3
      vch $v29, $v26, $v19
        suv $v28, 0, 16, $s0

      vcl $v29, $v27, $v20
      cfc2 $sp, $vcc
      andi $sp, $sp, 1799
      srl $t7, $sp, 5
    `);
    const cycles = linesToCycles(lines);
    expect(cycles).toEqual([
     1, 1,
     2, 2,
     3, 5,

     6,
     7,
     8+2,
     11
    ]);
  });

  // @TODO:
  // hardware-bug (VRCP, VRCPL, VRCPH, VMOV, VRSQ, VRSQL, VRSQH, VNOP)
});
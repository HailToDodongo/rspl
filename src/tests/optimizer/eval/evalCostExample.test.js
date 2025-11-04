import {evalFunctionCost} from "../../../lib/optimizer/eval/evalCost.js";
import {asm, asmLabel, asmNOP} from "../../../lib/intsructions/asmWriter.js";
import {
  asmInitDeps,
  asmScanDeps, OP_FLAG_IS_BRANCH,
  OP_FLAG_IS_LIKELY,
  OP_FLAG_LIKELY_BRANCH
} from "../../../lib/optimizer/asmScanDeps.js";

function textToAsmLines(text)
{
  const lines = text.trim().split("\n");
  return lines
      .map(line => line.substring(line.indexOf("]")+1))
      .map(line => line.trim())
      .filter(line => line.length > 0)
      .map(line => {
        const [op, ...args] = line.trim().split(" ");
        args.forEach((arg, i) => {
          if(arg.endsWith(","))args[i] = arg.slice(0, -1);
        });
        const res = op === "nop" ? asmNOP() : asm(op, args);
        if(res.opFlags & OP_FLAG_IS_BRANCH) {
          if(line.includes("unlikely")) {
            res.opFlags &= ~(OP_FLAG_LIKELY_BRANCH | OP_FLAG_IS_LIKELY);
          } else {
            res.opFlags |= (OP_FLAG_LIKELY_BRANCH | OP_FLAG_IS_LIKELY);
          }
        }
        return res;
      });
}

function textToAsmCycle(text) {
  const cycles = text.trim().split("\n")
    .map(line => line.trim())
    .filter(line => line.length > 0)
    .map(line =>
      line.substring(
        line.indexOf("[")+1,
        line.indexOf("]")).trim()
    );
  let lastCycle = 0;
  for(let i = 0; i < cycles.length; i++)
  {
    // count '*' amount
    const stars = (cycles[i].match(/\*/g) || []).length;
    if(!cycles[i].startsWith("^")) {
      lastCycle = parseInt(cycles[i]);
    } else {
      if(cycles[i-1])cycles[i-1] += stars;
    }
    lastCycle += stars;
    cycles[i] = lastCycle + 1;
  }
  return cycles;
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

describe('Eval - Cost (Examples)', () =>
{
  test('T3D Vertex Loop - 0', async () => {
    const code =
     `[0] nop
      [0] vmulf $v06, $v20, $v07.h0
      [1] ori $at, $zero, %lo(COLOR_AMBIENT)
      [^] vmacf $v06, $v19, $v07.h1
      [2] ori $s3, $zero, %lo(LIGHT_DIR_COLOR)
      [^] vmacf $v07, $v18, $v07.h2
      [3] vmudn $v06, $v28, $v08.h0
      [4] vmadh $v05, $v27, $v08.h0
      [5] vmadn $v06, $v26, $v08.h1
      [^] luv $v03, 0, 0, $at
      [6] vmadh $v05, $v25, $v08.h1
      [7] vmadn $v06, $v24, $v08.h2
      [8] vmadh $v05, $v23, $v08.h2
      [9] vmadn $v06, $v22, $v08.h3
      [^] luv $v04, 0, 16, $s4
      [10] vmadh $v05, $v21, $v08.h3
      [^] lpv $v08, 0, 8, $s3
      [11] beq $s3, $s2, LABEL_0003 # unlikely
      [12] nop
      [13]    luv $v01, 0, 0, $s3
      [^*]    vmulf $v02, $v07, $v08.v
      [15]    addiu $s3, $s3, 16
      [16]    lpv $v08, 0, 8, $s3
      [^**]   vmulu $v29, $v01, $v02.h0
      [19]    vmacu $v29, $v01, $v02.h1
      [20]    vmacu $v29, $v01, $v02.h2
      [^]     bne $s3, $s2, LABEL_0004 # unlikely
      [21***] vadd $v03, $v03, $v29.v
      [25] vmudl $v29, $v00, $v06.h3
      [26] vmadm $v29, $v15, $v06.h3
      [27] vmadn $v02, $v00, $v05.h3
      [^]  lqv $v08, 0, 32, $s4
      [28] vmadh $v01, $v15, $v05.h3
      [29] vch $v29, $v05, $v05.h3
      [30] vcl $v29, $v06, $v06.h3
      [31] cfc2 $t6, $vcc
      [32] addiu $s1, $s1, 72
      [ ^] vch $v29, $v05, $v01
      [33] vcl $v29, $v06, $v02
      [34] vmulf $v04, $v04, $v03.v
      [ ^] cfc2 $t5, $vcc
      [35] vmudl $v06, $v06, $v10.v
      [ ^] andi $t8, $t6, 1799
      [36] vmadm $v05, $v05, $v10.v
      [37] vmadn $v06, $v00, $v00
      [ ^] srl $t9, $t5, 4
      [38] andi $k0, $t5, 1799
      [39] srl $t4, $k0, 5
      [40] sdv $v05, 8, 16, $s5
      [41] sdv $v05, 0, 16, $s6
      [ ^] vrcph $v05.e3, $v05.e3
      [42] sdv $v06, 0, 24, $s6
      [43] sdv $v06, 8, 24, $s5
      [ ^] vrcpl $v06.e3, $v06.e3
      [44] andi $t9, $t9, 1799
      [45] or $k0, $k0, $t4
      [46] srl $t4, $t9, 5
      [ ^] vrcph $v05.e3, $v05.e7
      [47] vrcpl $v06.e7, $v06.e7
      [ ^] or $t9, $t9, $t4
      [48] srl $t4, $t8, 5
      [ ^] vrcph $v05.e7, $v00.e7
      [49] nor $t8, $t8, $t4
      [50] srl $t7, $t6, 4
      [ ^*] vaddc $v03, $v06, $v11.e1
      [ 52] vadd $v02, $v05, $v11.e0
      [  ^] ssv $v05, 6, 32, $s6    
      [ 53] andi $t8, $t8, 255    
      [ 54] suv $v04, 0, 8, $s6    
      [ 55] ssv $v05, 14, 32, $s5    
      [  ^] vmudn $v03, $v03, $v11.e3    
      [ 56] ldv $v03, 0, 24, $s4    
      [  ^] vmadh $v02, $v02, $v11.e3
      [ 57] ssv $v06, 14, 34, $s5   
      [ 58] addiu $s4, $s4, 32   
      [ 59] ssv $v06, 6, 34, $s6   
      [ 60] andi $t7, $t7, 1799   
      [  ^] vsub $v02, $v11, $v02.v   
      [ 61] sll $k0, $k0, 8   
      [  ^] vmudl $v29, $v06, $v06.h3   
      [ 62] srl $t4, $t7, 5   
      [  ^] vmadm $v29, $v05, $v06.h3   
      [ 63] vmadn $v06, $v06, $v05.h3
      [  ^] nor $t7, $t7, $t4   
      [ 64] vmadh $v05, $v05, $v05.h3   
      [  ^] mfc2 $sp, $v02.e6   
      [ 65] mfc2 $fp, $v02.e2   
      [^**] vmudl $v29, $v06, $v14.v 
      [ 68] vmadm $v29, $v05, $v14.v   
      [  ^] sra $sp, $sp, 7   
      [ 69] sra $fp, $fp, 7   
      [ ^*] vmadn $v06, $v06, $v13.v
      [ 71] vmadh $v05, $v05, $v13.v
      [ 72] vmadh $v05, $v12, $v30.e7   
      [  ^] suv $v04, 4, 8, $s5 
      [ 73] vor $v02, $v00, $v07   
      [  ^] sb $fp, -69($s1)  
      [ 74] vand $v07, $v17, $v08.h3   
      [  ^] or $k0, $k0, $t8   
      [ 75] sb $sp, -33($s1)   
      [ 76] sdv $v05, 0, 0, $s6   
      [ 77] sdv $v05, 8, 0, $s5   
      [ 78] sh $k0, 6($s6)
      [  ^] vmudn $v07, $v07, $v16.v   
      [ 79] sb $t9, 6($s5)   
      [  ^] vmov $v08.e3, $v30.e7
      [ 80] vmov $v08.e7, $v30.e7   
      [  ^] jal $k1   
      [81*] sb $t7, 7($s5)  
      [ 83] slv $v03, 4, 12, $s5  
      [ 84] slv $v03, 0, 12, $s6
      [ 85] addiu $s6, $s6, 72`;

    const lines = textToAsmLines(code);
    const cyclesExp = textToAsmCycle(code);

    const cycles = linesToCycles(lines);
    for(let line = 0; line < cycles.length; line++) {
      if(cycles[line] !== cyclesExp[line]) {
        const lines = code.split("\n");
        // get surrounding lines and mark the current one
        const contextSize = 8;
        const start = Math.max(0, line - contextSize);
        const end = Math.min(lines.length, line + 4);
        const context = lines.slice(start, end).map((l, i) => {
          if(i === contextSize) return `> ${l}`;
          return `  ${l}`;
        }).join("\n");
        throw new Error(`Cycle mismatch at line ${line}: expected ${cyclesExp[line]}, got ${cycles[line]}\n` + context);
        break;
      }
    }
  });
});
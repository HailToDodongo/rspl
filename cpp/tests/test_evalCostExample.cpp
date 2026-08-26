#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "optimizer/asm_scan_deps.h"
#include "optimizer/eval_cost.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

using namespace rspl;

// Parse text with bracket annotations like "[0] nop", "[^] vadd..." into
// AsmInst vectors.  Lines with "# unlikely" clear the likely-branch flags.
static std::vector<AsmInst> textToAsmLines(const std::string &text) {
  std::vector<AsmInst> lines;
  std::istringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    // strip leading bracket annotation [*]
    auto rb = line.find(']');
    if (rb == std::string::npos) continue;
    line = line.substr(rb + 1);

    // trim
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    size_t end = line.find_last_not_of(" \t");
    line = line.substr(start, end - start + 1);
    if (line.empty()) continue;

    // remove trailing comment
    auto hashPos = line.find('#');
    bool unlikely = false;
    if (hashPos != std::string::npos) {
      if (line.find("unlikely", hashPos) != std::string::npos)
        unlikely = true;
      line = line.substr(0, hashPos);
      // trim again
      end = line.find_last_not_of(" \t");
      if (end == std::string::npos) continue;
      line = line.substr(0, end + 1);
    }

    std::istringstream ls(line);
    std::string op;
    ls >> op;
    std::vector<std::string> args;
    std::string arg;
    while (ls >> arg) {
      if (arg.back() == ',') arg.pop_back();
      args.push_back(arg);
    }
    AsmInst inst;
    if (op == "nop")
      inst = asmNOP();
    else
      inst = asmOp(op, args);

    if (inst.opFlags & OpFlag::OP_FLAG_IS_BRANCH) {
      if (unlikely) {
        inst.opFlags &=
            ~(OpFlag::OP_FLAG_LIKELY_BRANCH | OpFlag::OP_FLAG_IS_LIKELY);
      } else {
        inst.opFlags |=
            (OpFlag::OP_FLAG_LIKELY_BRANCH | OpFlag::OP_FLAG_IS_LIKELY);
      }
    }
    lines.push_back(std::move(inst));
  }
  return lines;
}

// Parse the bracket annotations into expected cycle numbers.
static std::vector<int> textToAsmCycle(const std::string &text) {
  std::vector<std::string> annotations;
  std::istringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    auto lb = line.find('[');
    auto rb = line.find(']');
    if (lb == std::string::npos || rb == std::string::npos) continue;
    std::string a = line.substr(lb + 1, rb - lb - 1);
    // trim
    size_t s = a.find_first_not_of(" \t");
    if (s == std::string::npos) continue;
    size_t e = a.find_last_not_of(" \t");
    a = a.substr(s, e - s + 1);
    annotations.push_back(a);
  }

  std::vector<int> cycles;
  int lastCycle = 0;
  for (size_t i = 0; i < annotations.size(); ++i) {
    int stars = static_cast<int>(
        std::count(annotations[i].begin(), annotations[i].end(), '*'));
    if (!annotations[i].starts_with("^")) {
      lastCycle = std::stoi(annotations[i]);
    } else {
      if (i > 0) cycles[i - 1] += stars;
    }
    lastCycle += stars;
    cycles.push_back(lastCycle + 1);
  }
  return cycles;
}

static std::vector<int> linesToCycles(std::vector<AsmInst> &lines) {
  AsmFunc func;
  func.asm_ = std::move(lines);
  asmInitDeps(func);
  evalFunctionCost(func);
  std::vector<int> cycles;
  for (const auto &inst : func.asm_)
    cycles.push_back(inst.debug.cycle);
  return cycles;
}

static const std::string T3D_CODE = R"(
[0] nop
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
[ 85] addiu $s6, $s6, 72
)";

TEST_CASE("Eval - Cost (Examples) - T3D Vertex Loop - 0", "[evalCostExample]") {
  auto lines = textToAsmLines(T3D_CODE);
  auto cyclesExp = textToAsmCycle(T3D_CODE);

  auto cycles = linesToCycles(lines);

  REQUIRE(cycles.size() == cyclesExp.size());
  for (size_t line = 0; line < cycles.size(); ++line) {
    INFO("Line " << line);
    REQUIRE(cycles[line] == cyclesExp[line]);
  }
}

// ares-measured ground truth (00_quad single triangle, segment 0: entry pc 0x280, 173 instr, 97 cycles)
static const std::string TRI_RSPL_SEG0 = R"(
[  0] srl $a2, $a1, 16
[  1] lbu $v0, %lo(FACE_CULLING + 0)
[  2] mtc2 $a1, $v02.e3
[  3] llv $v02, 0, 0, $a1
[  4] llv $v01, 0, 0, $a0
[  4] vsubc $v27, $v00, $v30.v
[  5] vaddc $v12, $v00, $v30.e7
[  5] llv $v03, 0, 0, $a2
[  6] lhu $t5, 6($a2)
[  7] lhu $t4, 6($a1)
[  7] vor $v07, $v00, $v02.e1
[  8] vor $v06, $v00, $v01.e1
[  8] addiu $at, $zero, 255
[  9] lhu $t3, 6($a0)
[ 10] mtc2 $a0, $v01.e3
[ 11] lhu $s7, %lo(RDPQ_TRI_BUFF_OFFSET + 0)
[ 11] vor $v08, $v00, $v03.e1
[ 12] or $t3, $t3, $t4
[ 13] or $t3, $t3, $t5
[ 13] vge $v09, $v06, $v07
[ 14] vmrg $v04, $v01, $v02
[ 14] andi $t6, $t3, 255
[ 15] vlt $v06, $v06, $v07
[ 15] mtc2 $a2, $v03.e3
[ 16] cfc2 $t0, $vcc
[ 16] vmrg $v01, $v01, $v02
[ 17] bne $t6, $at, JrRa  # unlikely
[ 18] vxor $v28, $v28, $v28.v
[ 19] vge $v10, $v06, $v08
[ 20] vmrg $v05, $v01, $v03
[ 20] addiu $s3, $s7, %lo(CLIP_BUFFER_TMP)
[ 21] vlt $v06, $v06, $v08
[ 22] cfc2 $t1, $vcc
[ 22] vmrg $v01, $v01, $v03
[ 23] vge $v08, $v09, $v10
[ 23] andi $t4, $t3, 7936
[ 24] lw $a3, %lo(TRI_COMMAND + 0)
[ 24] vmrg $v03, $v04, $v05
[ 25] vlt $v07, $v09, $v10
[ 25] ssv $v06, 0, 6, $s3
[ 26] vmrg $v02, $v04, $v05
[ 26] bne $t4, $zero, RDPQ_Triangle_Clip  # unlikely
[ 27] cfc2 $t2, $vcc
[ 28] xor $t0, $t0, $t1
[ 28] vmudn $v26, $v06, $v31.e1
[ 29] mfc2 $a0, $v01.e3
[ 29] vsubc $v05, $v02, $v01.v
[ 31] vsubc $v04, $v03, $v02.v
[ 31] ssv $v07, 0, 4, $s3
[ 32] xor $t0, $t0, $t2
[ 32] vmudm $v25, $v06, $v31.e1
[ 33] xori $s7, $s7, 176
[ 33] vsubc $v24, $v03, $v01.v
[ 34] vsubc $v29, $v00, $v05.e1
[ 34] mfc2 $a2, $v03.e3
[ 35] vmov $v23.e0, $v01.e0
[ 35] mfc2 $a1, $v02.e3
[ 36] vmov $v23.e2, $v02.e0
[ 36] andi $t0, $t0, 1
[ 37] xor $t1, $v0, $t0
[ 37] vmudh $v19, $v05, $v24.e1
[ 38] slv $v05, 0, 8, $s3
[ 38] vmadh $v19, $v24, $v29.v
[ 39] ssv $v08, 0, 2, $s3
[ 39] vsar $v18, COP2_ACC_HI
[ 40] slv $v04, 0, 12, $s3
[ 40] vsar $v19, COP2_ACC_MD
[ 41] ldv $v24, 4, 8, $s3
[ 41] vsubc $v26, $v00, $v26.v
[ 42] addiu $t4, $s3, 32
[ 42] vsub $v25, $v25, $v25.v
[ 43] vmov $v28.e7, $v18.e0
[ 43] lsv $v08, 0, 30, $a0
[ 44] mfc2 $t0, $v18.e0
[ 44] vmov $v24.e7, $v19.e0
[ 45] lsv $v08, 8, 30, $a2
[ 45] vrcph $v20.e7, $v18.e0
[ 46] lsv $v08, 4, 30, $a1
[ 46] vrcpl $v21.e7, $v19.e0
[ 47] slt $t0, $t0, $zero
[ 47] vrcph $v20.e7, $v00.e0
[ 48] vrcp $v21.e0, $v24.e1
[ 48] beq $t0, $t1, JrRa  # unlikely
[ 49] vrcph $v20.e0, $v24.e1
[ 50] lsv $v07, 8, 22, $a2
[ 50] vrcp $v21.e2, $v05.e1
[ 51] lsv $v07, 0, 22, $a0
[ 51] vrcph $v20.e2, $v05.e1
[ 52] luv $v10, 0, 8, $a1
[ 52] vrcp $v21.e4, $v04.e1
[ 53] luv $v11, 0, 8, $a2
[ 53] vrcph $v20.e4, $v04.e1
[ 54] lsv $v15, 10, 34, $a1
[ 55] lsv $v07, 4, 22, $a1
[ 56] xori $t0, $t0, 1
[ 56] vmudn $v01, $v21, $v27.e5
[ 57] vmadh $v02, $v20, $v27.e5
[ 57] lsv $v15, 8, 34, $a0
[ 58] vsubc $v29, $v08, $v08.e2
[ 58] sll $t0, $t0, 7
[ 59] vlt $v07, $v07, $v07.e2
[ 59] or $a3, $a3, $t0
[ 60] vmrg $v08, $v08, $v08.e2
[ 60] andi $t5, $a3, 1024
[ 61] luv $v09, 0, 8, $a0
[ 61] vmudl $v03, $v21, $v24.q1
[ 62] vmadm $v03, $v20, $v24.q1
[ 62] andi $t6, $a3, 512
[ 63] vmadn $v03, $v21, $v28.v
[ 63] srl $t5, $t5, 4
[ 64] vmadh $v04, $v20, $v28.v
[ 64] mfc2 $t0, $v24.e3
[ 65] vmadn $v03, $v12, $v27.e7
[ 65] lsv $v15, 12, 34, $a2
[ 66] vmadh $v04, $v12, $v27.e7
[ 66] addiu $at, $zero, 207
[ 67] vsubc $v29, $v08, $v08.e4
[ 67] addu $t5, $t5, $t4
[ 68] vlt $v07, $v07, $v07.e4
[ 68] lsv $v14, 12, 32, $a2
[ 69] mfc2 $t1, $v24.e0
[ 69] vmrg $v08, $v08, $v08.e4
[ 70] vmudl $v29, $v03, $v01
[ 70] lsv $v14, 8, 32, $a0
[ 71] vmadm $v29, $v04, $v01
[ 71] lsv $v14, 10, 32, $a1
[ 72] vmadn $v19, $v03, $v02.v
[ 72] srl $t6, $t6, 3
[ 73] vmadh $v18, $v04, $v02.v
[ 73] llv $v04, 8, 12, $a0
[ 74] vmudl $v29, $v15, $v08.e0
[ 74] llv $v06, 8, 12, $a2
[ 75] vmadm $v29, $v14, $v08.e0
[ 75] srl $t2, $t3, 13
[ 76] or $a3, $a3, $t2
[ 76] vmadn $v15, $v15, $v07.e0
[ 77] vmudn $v29, $v19, $v24.v
[ 77] ctc2 $at, $vcc
[ 78] vmadh $v29, $v18, $v24.v
[ 78] sh $a3, 0($s3)
[ 79] vsar $v21, COP2_ACC_MD
[ 79] subu $t1, $zero, $t1
[ 80] vsar $v20, COP2_ACC_HI
[ 80] addu $t6, $t6, $t5
[ 81] vsubc $v15, $v15, $v30.e7
[ 81] mtc2 $t1, $v24.e0
[ 82] vmudl $v10, $v10, $v31.e6
[ 82] mfc0 $t1, COP0_DMA_BUSY
[ 83] vmudl $v09, $v09, $v31.e6
[ 83] lsv $v10, 14, 4, $a1
[ 84] vmudl $v11, $v11, $v31.e6
[ 84] llv $v03, 8, 12, $a1
[ 85] lsv $v11, 14, 4, $a2
[ 85] vmudl $v15, $v15, $v31.e0
[ 86] vmudl $v29, $v21, $v26.e4
[ 86] lsv $v09, 14, 4, $a0
[ 87] andi $t3, $a3, 256
[ 87] vmadm $v29, $v20, $v26.e4
[ 88] vmadn $v17, $v21, $v25.e4
[ 88] subu $t0, $zero, $t0
[ 89] vmadh $v16, $v20, $v25.e4
[ 89] mtc2 $t0, $v24.e3
[ 90] sdv $v15, 8, 16, $s3
[ 90] vmulf $v06, $v06, $v15.h2
[ 91] vmulf $v04, $v04, $v15.h0
[ 91] lbu $t0, %lo(RDPQ_SYNCFULL_ONGOING + 0)
[ 92] lsv $v09, 12, 16, $s3
[ 92] vmulf $v03, $v03, $v15.h1
[ 93] lsv $v10, 12, 18, $s3
[ 94] lsv $v11, 12, 20, $s3
[ 94] vmudm $v22, $v23, $v31.e1
[ 95] beq $t1, $zero, LABEL_RDPQ_Triangle_Send_Async_0004  # unlikely
[ 96] vmudn $v23, $v23, $v31.e1
)";

TEST_CASE("Eval - Cost (Examples) - TRI RSPL seg0", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_RSPL_SEG0);
  auto cyclesExp = textToAsmCycle(TRI_RSPL_SEG0);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  // The final (total) cycle must match ares exactly. Per-line, short
  // 1-2 line divergences are tolerated: eval charges a stall BEFORE the
  // stalled instruction, while the hardware trace logs the group before
  // its own stall cycles are added - same totals, shifted attribution.
  // Known transient sites (2026-08-26): mtc2-after-loads stacking,
  // mfc2 pair shift, and the unmodeled taken-branch target alignment
  // rule (ares rsp.cpp: "if(branch.pc & 4) pipeline.singleIssue = 1").
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) +
                ": model=" + std::to_string(cycles[line]) +
                " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}

// ares-measured ground truth (00_quad single triangle, segment 1: entry pc 0x540, 12 instr, 7 cycles)
static const std::string TRI_RSPL_SEG1 = R"(
[  0] vmrg $v10, $v10, $v03
[  0] srl $t3, $t3, 4
[  1] ssv $v22, 4, 8, $s3
[  2] ssv $v23, 4, 10, $s3
[  2] vmrg $v11, $v11, $v06
[  3] vmrg $v09, $v09, $v04
[  3] lw $a0, %lo(RDPQ_CURRENT + 0)
[  4] ssv $v20, 8, 12, $s3
[  4] vaddc $v17, $v17, $v23.e0
[  5] vadd $v16, $v16, $v22.e0
[  5] bne $t0, $zero, LABEL_RDPQ_Triangle_Send_Async_0007  # unlikely
[  6] ssv $v21, 8, 14, $s3
)";

TEST_CASE("Eval - Cost (Examples) - TRI RSPL seg1", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_RSPL_SEG1);
  auto cyclesExp = textToAsmCycle(TRI_RSPL_SEG1);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  // The final (total) cycle must match ares exactly. Per-line, short
  // 1-2 line divergences are tolerated: eval charges a stall BEFORE the
  // stalled instruction, while the hardware trace logs the group before
  // its own stall cycles are added - same totals, shifted attribution.
  // Known transient sites (2026-08-26): mtc2-after-loads stacking,
  // mfc2 pair shift, and the unmodeled taken-branch target alignment
  // rule (ares rsp.cpp: "if(branch.pc & 4) pipeline.singleIssue = 1").
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) +
                ": model=" + std::to_string(cycles[line]) +
                " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}

// ares-measured ground truth (00_quad single triangle, segment 2: entry pc 0x574, 48 instr, 26 cycles)
static const std::string TRI_RSPL_SEG2 = R"(
[  0] vsubc $v29, $v00, $v00
[  1] vsub $v12, $v10, $v09.v
[  1] lw $a2, %lo(RDPQ_SENTINEL + 0)
[  2] vsub $v13, $v11, $v09.v
[  2] addu $t3, $t3, $t6
[  3] vmudn $v19, $v19, $v30.e5
[  3] subu $t3, $t3, $s3
[  4] vmadh $v18, $v18, $v30.e5
[  4] ssv $v16, 0, 16, $s3
[  5] vmudh $v29, $v12, $v24.e1
[  5] ssv $v17, 0, 18, $s3
[  6] vmadh $v29, $v13, $v24.e3
[  6] ssv $v20, 0, 20, $s3
[  7] vsar $v04, COP2_ACC_MD
[  7] ssv $v21, 0, 22, $s3
[  8] ssv $v20, 4, 28, $s3
[  8] vsar $v03, COP2_ACC_HI
[  9] vmudh $v29, $v13, $v24.e2
[  9] ssv $v16, 4, 24, $s3
[ 10] vmadh $v29, $v12, $v24.e0
[ 11] vsar $v08, COP2_ACC_MD
[ 12] vsar $v07, COP2_ACC_HI
[ 12] ssv $v17, 4, 26, $s3
[ 13] sh $s7, %lo(RDPQ_TRI_BUFF_OFFSET)($zero)
[ 13] vmudl $v29, $v04, $v19.e7
[ 14] vmadm $v29, $v03, $v19.e7
[ 14] ssv $v21, 4, 30, $s3
[ 15] mtc0 $s3, COP0_DMA_SPADDR
[ 15] vmadn $v04, $v04, $v18.e7
[ 16] mtc0 $a0, COP0_DMA_RAMADDR
[ 16] vmadh $v03, $v03, $v18.e7
[ 17] vmudl $v29, $v08, $v19.e7
[ 17] addu $a0, $a0, $t3
[ 18] vmadm $v29, $v07, $v19.e7
[ 18] sltu $at, $a2, $a0
[ 19] addu $s3, $s3, $t3
[ 19] vmadn $v08, $v08, $v18.e7
[ 20] vmadh $v07, $v07, $v18.e7
[ 20] sdv $v03, 8, 8, $t5
[ 21] vmadl $v29, $v04, $v21.e0
[ 21] sdv $v03, 0, 8, $t4
[ 22] vmadm $v29, $v03, $v21.e0
[ 22] sdv $v04, 8, 24, $t5
[ 23] sdv $v08, 0, 56, $t4
[ 23] vmadn $v06, $v04, $v20.e0
[ 24] vmadh $v05, $v03, $v20.e0
[ 24] beq $at, $zero, LABEL_RDPQ_Triangle_Send_Async_0008  # unlikely
[ 25] sdv $v04, 0, 24, $t4
)";

TEST_CASE("Eval - Cost (Examples) - TRI RSPL seg2", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_RSPL_SEG2);
  auto cyclesExp = textToAsmCycle(TRI_RSPL_SEG2);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  // The final (total) cycle must match ares exactly. Per-line, short
  // 1-2 line divergences are tolerated: eval charges a stall BEFORE the
  // stalled instruction, while the hardware trace logs the group before
  // its own stall cycles are added - same totals, shifted attribution.
  // Known transient sites (2026-08-26): mtc2-after-loads stacking,
  // mfc2 pair shift, and the unmodeled taken-branch target alignment
  // rule (ares rsp.cpp: "if(branch.pc & 4) pipeline.singleIssue = 1").
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) +
                ": model=" + std::to_string(cycles[line]) +
                " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}

// ares-measured ground truth (00_quad single triangle, segment 3: entry pc 0x66c, 29 instr, 19 cycles)
static const std::string TRI_RSPL_SEG3 = R"(
[  0] vmudh $v29, $v09, $v30.e7
[  1] sw $a0, %lo(RDPQ_CURRENT)($zero)
[  1] vmadl $v29, $v06, $v26.e4
[  2] vmadm $v29, $v05, $v26.e4
[  2] sdv $v07, 0, 40, $t4
[  3] addiu $t3, $t3, 65535
[  3] vmadn $v02, $v06, $v25.e4
[  4] sdv $v06, 8, 48, $t5
[  4] vmadh $v01, $v05, $v25.e4
[  5] sdv $v06, 0, 48, $t4
[  5] vmov $v10.e3, $v04.e7
[  6] sdv $v08, 8, 56, $t5
[  6] vmov $v06.e6, $v05.e7
[  7] vmov $v08.e6, $v07.e7
[  7] sdv $v05, 0, 32, $t4
[  8] vmov $v10.e0, $v01.e7
[  8] sdv $v02, 8, 16, $t5
[  9] vmov $v10.e1, $v02.e7
[  9] sdv $v02, 0, 16, $t4
[ 10] vmov $v10.e2, $v03.e7
[ 10] sdv $v07, 8, 40, $t5
[ 11] sdv $v01, 0, 0, $t4
[ 12] slv $v08, 12, 12, $t6
[ 13] sdv $v05, 8, 32, $t5
[ 14] sdv $v01, 8, 0, $t5
[ 15] slv $v06, 12, 8, $t6
[ 16] sdv $v10, 0, 0, $t6
[ 17] jr $ra  # unlikely
[ 18] mtc0 $t3, COP0_DMA_WRITE
)";

TEST_CASE("Eval - Cost (Examples) - TRI RSPL seg3", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_RSPL_SEG3);
  auto cyclesExp = textToAsmCycle(TRI_RSPL_SEG3);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  // The final (total) cycle must match ares exactly. Per-line, short
  // 1-2 line divergences are tolerated: eval charges a stall BEFORE the
  // stalled instruction, while the hardware trace logs the group before
  // its own stall cycles are added - same totals, shifted attribution.
  // Known transient sites (2026-08-26): mtc2-after-loads stacking,
  // mfc2 pair shift, and the unmodeled taken-branch target alignment
  // rule (ares rsp.cpp: "if(branch.pc & 4) pipeline.singleIssue = 1").
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) +
                ": model=" + std::to_string(cycles[line]) +
                " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}

// ares-measured ground truth (00_quad single triangle, segment 0: entry pc 0x280, 184 instr, 103 cycles)
static const std::string TRI_REF_SEG0 = R"(
[  0] srl $a2, $a1, 16
[  1] lbu $v0, 1016($zero)
[  2] mtc2 $a0, $v01.e3
[  3] mtc2 $a1, $v02.e3
[  6] llv $v01, 0, 0, $a0
[  7] llv $v02, 0, 0, $a1
[  8] llv $v03, 0, 0, $a2
[  9] lhu $t3, 6($a0)
[  9] vsubc $v27, $v00, $v30
[ 10] vor $v06, $v00, $v01.e1
[ 10] ori $t1, $zero, 0xFF
[ 11] vor $v07, $v00, $v02.e1
[ 11] lhu $t4, 6($a1)
[ 12] vor $v08, $v00, $v03.e1
[ 12] mtc2 $a2, $v03.e3
[ 13] vaddc $v12, $v00, $v30.e7
[ 13] lhu $t5, 6($a2)
[ 14] lhu $s7, 836($zero)
[ 14] vge $v09, $v06, $v07
[ 16] or $t3, $t3, $t4
[ 16] vmrg $v04, $v01, $v02
[ 17] or $t3, $t3, $t5
[ 17] vlt $v06, $v06, $v07
[ 18] cfc2 $t0, $vcc
[ 18] vmrg $v01, $v01, $v02
[ 19] andi $at, $t3, 0xFF
[ 19] vxor $v28, $v28, $v28
[ 20] bne $at, $t1, LABEL_260  # unlikely
[ 21] vge $v10, $v06, $v08
[ 22] srl $at, $t3, 13
[ 22] vmrg $v05, $v01, $v03
[ 23] lw $a3, 1008($zero)
[ 23] vlt $v06, $v06, $v08
[ 24] cfc2 $t1, $vcc
[ 24] vmrg $v01, $v01, $v03
[ 25] andi $t4, $t3, 0x1F00
[ 25] vge $v08, $v09, $v10
[ 26] addiu $s3, $s7, 0xDE0
[ 26] vmrg $v03, $v04, $v05
[ 27] ssv $v06, 0, 6, $s3
[ 27] vlt $v07, $v09, $v10
[ 28] cfc2 $t2, $vcc
[ 28] vmrg $v02, $v04, $v05
[ 29] bne $t4, $zero, LABEL_EA0  # unlikely
[ 30] xor $t0, $t0, $t1
[ 31] vsubc $v24, $v03, $v01
[ 31] xor $t0, $t0, $t2
[ 32] vsubc $v05, $v02, $v01
[ 32] mfc2 $a0, $v01.e3
[ 33] vsubc $v04, $v03, $v02
[ 33] mfc2 $a1, $v02.e3
[ 34] vmov $v23.e0, $v01.e0
[ 34] andi $t0, $t0, 0x1
[ 35] vmov $v23.e2, $v02.e0
[ 35] xor $t1, $v0, $t0
[ 36] vsubc $v29, $v00, $v05.e1
[ 36] mfc2 $a2, $v03.e3
[ 37] vmudn $v26, $v06, $v31.e1
[ 37] ssv $v07, 0, 4, $s3
[ 38] vmudm $v25, $v06, $v31.e1
[ 38] xori $s7, $s7, 0xB0
[ 39] vmudh $v19, $v05, $v24.e1
[ 39] ssv $v08, 0, 2, $s3
[ 40] vmadh $v19, $v24, $v29
[ 40] slv $v05, 0, 8, $s3
[ 41] vsar $v18, COP2_ACC_HI
[ 41] slv $v04, 0, 12, $s3
[ 42] vsar $v19, COP2_ACC_MD
[ 42] ldv $v24, 4, 8, $s3
[ 43] vsubc $v26, $v00, $v26
[ 43] addiu $t4, $s3, 0x20
[ 44] vsub $v25, $v25, $v25
[ 44] lsv $v08, 0, 30, $a0
[ 45] vrcph $v20.e7, $v18.e0
[ 45] mfc2 $t0, $v18.e0
[ 46] vrcpl $v21.e7, $v19.e0
[ 46] lsv $v08, 4, 30, $a1
[ 47] vrcph $v20.e7, $v00.e0
[ 47] lsv $v08, 8, 30, $a2
[ 48] vrcp $v21.e0, $v24.e1
[ 48] slt $t0, $t0, $zero
[ 49] vrcph $v20.e0, $v24.e1
[ 49] beq $t0, $t1, LABEL_260  # unlikely
[ 50] vrcp $v21.e2, $v05.e1
[ 51] vrcph $v20.e2, $v05.e1
[ 51] lsv $v07, 0, 22, $a0
[ 52] vrcp $v21.e4, $v04.e1
[ 52] lsv $v07, 4, 22, $a1
[ 53] vrcph $v20.e4, $v04.e1
[ 53] lsv $v07, 8, 22, $a2
[ 54] vmov $v24.e7, $v19.e0
[ 54] xori $t0, $t0, 0x1
[ 55] vmov $v28.e7, $v18.e0
[ 55] sll $t0, $t0, 7
[ 56] vmudn $v01, $v21, $v27.e5
[ 56] luv $v11, 0, 8, $a2
[ 57] vmadh $v02, $v20, $v27.e5
[ 57] luv $v09, 0, 8, $a0
[ 58] vsubc $v29, $v08, $v08.e2
[ 58] luv $v10, 0, 8, $a1
[ 59] vlt $v07, $v07, $v07.e2
[ 59] or $a3, $a3, $t0
[ 60] vmrg $v08, $v08, $v08.e2
[ 60] lsv $v15, 12, 34, $a2
[ 61] vmudl $v03, $v21, $v24.q1
[ 61] lsv $v15, 8, 34, $a0
[ 62] vmadm $v03, $v20, $v24.q1
[ 62] lsv $v15, 10, 34, $a1
[ 63] vmadn $v03, $v21, $v28
[ 63] andi $t5, $a3, 0x400
[ 64] vmadh $v04, $v20, $v28
[ 64] srl $t5, $t5, 4
[ 65] vmadn $v03, $v12, $v27.e7
[ 65] mfc2 $t0, $v24.e3
[ 66] vmadh $v04, $v12, $v27.e7
[ 66] mfc2 $t1, $v24.e0
[ 67] vsubc $v29, $v08, $v08.e4
[ 67] lsv $v14, 10, 32, $a1
[ 68] vlt $v07, $v07, $v07.e4
[ 68] lsv $v14, 12, 32, $a2
[ 69] vmrg $v08, $v08, $v08.e4
[ 69] lsv $v14, 8, 32, $a0
[ 70] vmudl $v29, $v03, $v01
[ 70] addu $t5, $t5, $t4
[ 71] vmadm $v29, $v04, $v01
[ 71] andi $t6, $a3, 0x200
[ 72] vmadn $v19, $v03, $v02
[ 72] or $a3, $a3, $at
[ 73] vmadh $v18, $v04, $v02
[ 73] sh $a3, 0($s3)
[ 74] vmudl $v29, $v15, $v08.e0
[ 74] llv $v04, 8, 12, $a0
[ 75] vmadm $v29, $v14, $v08.e0
[ 75] llv $v03, 8, 12, $a1
[ 76] vmadn $v15, $v15, $v07.e0
[ 76] llv $v06, 8, 12, $a2
[ 77] vmudn $v29, $v19, $v24
[ 77] sub $t0, $zero, $t0
[ 78] vmadh $v29, $v18, $v24
[ 78] sub $t1, $zero, $t1
[ 79] vsar $v21, COP2_ACC_MD
[ 79] mtc2 $t0, $v24.e3
[ 80] vsar $v20, COP2_ACC_HI
[ 80] mtc2 $t1, $v24.e0
[ 81] vsubc $v15, $v15, $v30.e7
[ 81] addiu $t0, $zero, 0x000000CF
[ 82] vmudl $v09, $v09, $v31.e6
[ 83] lsv $v09, 14, 4, $a0
[ 83] vmudl $v10, $v10, $v31.e6
[ 84] ctc2 $t0, $vcc
[ 84] vmudl $v11, $v11, $v31.e6
[ 85] lsv $v10, 14, 4, $a1
[ 85] vmudl $v15, $v15, $v31.e0
[ 86] lsv $v11, 14, 4, $a2
[ 86] vmudl $v29, $v21, $v26.e4
[ 87] srl $t6, $t6, 3
[ 87] vmadm $v29, $v20, $v26.e4
[ 88] andi $t3, $a3, 0x100
[ 88] vmadn $v17, $v21, $v25.e4
[ 89] mfc0 $t1, COP0_DMA_BUSY
[ 89] vmadh $v16, $v20, $v25.e4
[ 90] sdv $v15, 0, 16, $s3
[ 90] vmulf $v04, $v04, $v15.h0
[ 91] lsv $v09, 12, 16, $s3
[ 91] vmulf $v03, $v03, $v15.h1
[ 92] lsv $v10, 12, 18, $s3
[ 92] vmulf $v06, $v06, $v15.h2
[ 93] lsv $v11, 12, 20, $s3
[ 93] vmudm $v22, $v23, $v31.e1
[ 94] bne $t1, $zero, LABEL_6D8  # unlikely
[ 95] lbu $t0, 481($zero)
[ 96] vmudn $v23, $v23, $v31.e1
[ 96] srl $t3, $t3, 4
[ 97] vmrg $v09, $v09, $v04
[ 97] addu $t6, $t6, $t5
[ 98] vmrg $v10, $v10, $v03
[ 98] ssv $v20, 0, 12, $s3
[ 99] vmrg $v11, $v11, $v06
[ 99] lw $a0, 464($zero)
[100] vaddc $v17, $v17, $v23.e0
[100] ssv $v22, 0, 8, $s3
[101] vadd $v16, $v16, $v22.e0
[101] bne $t0, $zero, LABEL_564  # unlikely
[102] ssv $v21, 0, 14, $s3
)";

TEST_CASE("Eval - Cost (Examples) - TRI REF seg0", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_REF_SEG0);
  auto cyclesExp = textToAsmCycle(TRI_REF_SEG0);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  // The final (total) cycle must match ares exactly. Per-line, short
  // 1-2 line divergences are tolerated: eval charges a stall BEFORE the
  // stalled instruction, while the hardware trace logs the group before
  // its own stall cycles are added - same totals, shifted attribution.
  // Known transient sites (2026-08-26): mtc2-after-loads stacking,
  // mfc2 pair shift, and the unmodeled taken-branch target alignment
  // rule (ares rsp.cpp: "if(branch.pc & 4) pipeline.singleIssue = 1").
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) +
                ": model=" + std::to_string(cycles[line]) +
                " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}

// ares-measured ground truth (00_quad single triangle, segment 1: entry pc 0x564, 78 instr, 43 cycles)
static const std::string TRI_REF_SEG1 = R"(
[  0] vsubc $v00, $v00, $v00
[  1] vsub $v12, $v10, $v09
[  1] ssv $v23, 0, 10, $s3
[  2] vsub $v13, $v11, $v09
[  2] addu $t3, $t3, $t6
[  3] vmudn $v19, $v19, $v30.e5
[  3] ssv $v20, 0, 28, $s3
[  4] vmadh $v18, $v18, $v30.e5
[  4] ssv $v21, 0, 30, $s3
[  5] vmudh $v04, $v12, $v24.e1
[  5] ssv $v16, 0, 24, $s3
[  6] vmadh $v04, $v13, $v24.e3
[  6] ssv $v17, 0, 26, $s3
[  7] vsar $v04, COP2_ACC_MD
[  7] ssv $v16, 0, 16, $s3
[  8] vsar $v03, COP2_ACC_HI
[  8] ssv $v17, 0, 18, $s3
[  9] vmudh $v08, $v13, $v24.e2
[  9] ssv $v20, 0, 20, $s3
[ 10] vmadh $v08, $v12, $v24.e0
[ 10] ssv $v21, 0, 22, $s3
[ 11] vsar $v08, COP2_ACC_MD
[ 11] mtc0 $s3, COP0_DMA_SPADDR
[ 12] vsar $v07, COP2_ACC_HI
[ 12] lw $a2, 468($zero)
[ 13] vmudl $v29, $v04, $v19.e7
[ 13] sub $t3, $t3, $s3
[ 14] vmadm $v29, $v03, $v19.e7
[ 14] addu $s3, $s3, $t3
[ 15] vmadn $v04, $v04, $v18.e7
[ 15] mtc0 $a0, COP0_DMA_RAMADDR
[ 16] vmadh $v03, $v03, $v18.e7
[ 16] sh $s7, 836($zero)
[ 17] vmudl $v29, $v08, $v19.e7
[ 17] add $a0, $a0, $t3
[ 18] vmadm $v29, $v07, $v19.e7
[ 18] slt $t0, $a2, $a0
[ 19] vmadn $v08, $v08, $v18.e7
[ 19] sdv $v04, 0, 24, $t4
[ 20] vmadh $v07, $v07, $v18.e7
[ 20] sdv $v04, 0, 24, $t5
[ 21] vmadl $v29, $v04, $v21.e0
[ 21] sdv $v03, 0, 8, $t4
[ 22] vmadm $v29, $v03, $v21.e0
[ 22] sdv $v03, 0, 8, $t5
[ 23] vmadn $v06, $v04, $v20.e0
[ 23] sdv $v08, 0, 56, $t4
[ 24] vmadh $v05, $v03, $v20.e0
[ 24] bne $t0, $zero, LABEL_69C  # unlikely
[ 25] sdv $v08, 0, 56, $t5
[ 26] vmudh $v29, $v09, $v30.e7
[ 26] sdv $v07, 0, 40, $t4
[ 27] vmadl $v29, $v06, $v26.e4
[ 27] sdv $v07, 0, 40, $t5
[ 28] vmadm $v29, $v05, $v26.e4
[ 28] sdv $v06, 0, 48, $t4
[ 29] vmadn $v02, $v06, $v25.e4
[ 29] sdv $v06, 0, 48, $t5
[ 30] vmadh $v01, $v05, $v25.e4
[ 30] sdv $v05, 0, 32, $t4
[ 31] vmov $v08.e6, $v07.e7
[ 31] sdv $v05, 0, 32, $t5
[ 32] vmov $v06.e6, $v05.e7
[ 32] sw $a0, 464($zero)
[ 33] vmov $v10.e1, $v02.e7
[ 33] sdv $v02, 0, 16, $t4
[ 34] vmov $v10.e0, $v01.e7
[ 34] sdv $v02, 0, 16, $t5
[ 35] vmov $v10.e3, $v04.e7
[ 35] sdv $v01, 0, 0, $t4
[ 36] vmov $v10.e2, $v03.e7
[ 36] sdv $v01, 0, 0, $t5
[ 37] slv $v06, 0, 8, $t6
[ 38] slv $v08, 0, 12, $t6
[ 39] addiu $t3, $t3, -0x1
[ 40] sdv $v10, 0, 0, $t6
[ 41] jr $ra  # unlikely
[ 42] mtc0 $t3, COP0_DMA_WRITE
)";

TEST_CASE("Eval - Cost (Examples) - TRI REF seg1", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_REF_SEG1);
  auto cyclesExp = textToAsmCycle(TRI_REF_SEG1);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  // The final (total) cycle must match ares exactly. Per-line, short
  // 1-2 line divergences are tolerated: eval charges a stall BEFORE the
  // stalled instruction, while the hardware trace logs the group before
  // its own stall cycles are added - same totals, shifted attribution.
  // Known transient sites (2026-08-26): mtc2-after-loads stacking,
  // mfc2 pair shift, and the unmodeled taken-branch target alignment
  // rule (ares rsp.cpp: "if(branch.pc & 4) pipeline.singleIssue = 1").
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) +
                ": model=" + std::to_string(cycles[line]) +
                " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}


// ares-measured ground truth (00_quad, @Unlikely layout, 158-cycle schedule, segment 0: entry pc 0x280, 185 instr, 104 cycles)
static const std::string TRI_RSPL2_SEG0 = R"(
[  0] srl $a2, $a1, 16
[  1] lbu $v0, %lo(FACE_CULLING + 0)
[  1] vsubc $v27, $v00, $v30.v
[  2] mtc2 $a1, $v02.e3
[  3] llv $v01, 0, 0, $a0
[  4] llv $v02, 0, 0, $a1
[  5] llv $v03, 0, 0, $a2
[  6] lhu $t4, 6($a1)
[  7] lhu $t3, 6($a0)
[  7] vor $v06, $v00, $v01.e1
[  8] addiu $at, $zero, 255
[  8] vor $v07, $v00, $v02.e1
[  9] lhu $t5, 6($a2)
[ 10] mtc2 $a0, $v01.e3
[ 11] or $t3, $t3, $t4
[ 11] vaddc $v12, $v00, $v30.e7
[ 12] vge $v09, $v06, $v07
[ 12] lhu $s7, %lo(RDPQ_TRI_BUFF_OFFSET + 0)
[ 13] vor $v08, $v00, $v03.e1
[ 13] mtc2 $a2, $v03.e3
[ 14] vmrg $v04, $v01, $v02
[ 14] or $t3, $t3, $t5
[ 15] vlt $v06, $v06, $v07
[ 15] andi $t6, $t3, 255
[ 16] cfc2 $t0, $vcc
[ 16] vmrg $v01, $v01, $v02
[ 17] bne $t6, $at, JrRa  # unlikely
[ 18] vxor $v28, $v28, $v28.v
[ 19] vge $v10, $v06, $v08
[ 19] addiu $s3, $s7, %lo(CLIP_BUFFER_TMP)
[ 20] vmrg $v05, $v01, $v03
[ 21] vlt $v06, $v06, $v08
[ 22] cfc2 $t1, $vcc
[ 22] vmrg $v01, $v01, $v03
[ 23] vge $v08, $v09, $v10
[ 23] andi $t4, $t3, 7936
[ 24] vmrg $v03, $v04, $v05
[ 25] vlt $v07, $v09, $v10
[ 25] ssv $v06, 0, 6, $s3
[ 26] vmrg $v02, $v04, $v05
[ 27] cfc2 $t2, $vcc
[ 28] bne $t4, $zero, RDPQ_Triangle_Clip  # unlikely
[ 29] lw $a3, %lo(TRI_COMMAND + 0)
[ 30] xor $t0, $t0, $t1
[ 30] vsubc $v05, $v02, $v01.v
[ 31] xor $t0, $t0, $t2
[ 31] vmov $v23.e0, $v01.e0
[ 32] mfc2 $a1, $v02.e3
[ 32] vmov $v23.e2, $v02.e0
[ 33] vsubc $v04, $v03, $v02.v
[ 33] mfc2 $a2, $v03.e3
[ 34] xori $s7, $s7, 176
[ 34] vsubc $v24, $v03, $v01.v
[ 35] andi $t0, $t0, 1
[ 35] vsubc $v29, $v00, $v05.e1
[ 36] slv $v05, 0, 8, $s3
[ 36] vmudm $v25, $v06, $v31.e1
[ 37] mfc2 $a0, $v01.e3
[ 37] vmudn $v26, $v06, $v31.e1
[ 38] vmudh $v19, $v05, $v24.e1
[ 38] ssv $v08, 0, 2, $s3
[ 39] vmadh $v19, $v24, $v29.v
[ 39] xor $t1, $v0, $t0
[ 40] slv $v04, 0, 12, $s3
[ 40] vsar $v18, COP2_ACC_HI
[ 41] ldv $v24, 4, 8, $s3
[ 41] vsar $v19, COP2_ACC_MD
[ 42] ssv $v07, 0, 4, $s3
[ 42] vsubc $v26, $v00, $v26.v
[ 43] addiu $t4, $s3, 32
[ 43] vsub $v25, $v25, $v25.v
[ 44] vmov $v28.e7, $v18.e0
[ 44] mfc2 $t0, $v18.e0
[ 45] vmov $v24.e7, $v19.e0
[ 45] lsv $v08, 8, 30, $a2
[ 46] lsv $v08, 0, 30, $a0
[ 46] vrcph $v20.e7, $v18.e0
[ 47] vrcpl $v21.e7, $v19.e0
[ 47] lsv $v08, 4, 30, $a1
[ 48] slt $t0, $t0, $zero
[ 48] vrcph $v20.e7, $v00.e0
[ 49] vrcp $v21.e0, $v24.e1
[ 49] beq $t0, $t1, JrRa  # unlikely
[ 50] vrcph $v20.e0, $v24.e1
[ 51] xori $t0, $t0, 1
[ 51] vrcp $v21.e2, $v05.e1
[ 52] lsv $v07, 0, 22, $a0
[ 52] vrcph $v20.e2, $v05.e1
[ 53] vrcp $v21.e4, $v04.e1
[ 53] lsv $v07, 4, 22, $a1
[ 54] lsv $v07, 8, 22, $a2
[ 54] vrcph $v20.e4, $v04.e1
[ 55] sll $t0, $t0, 7
[ 56] lsv $v15, 12, 34, $a2
[ 57] or $a3, $a3, $t0
[ 57] vmudn $v01, $v21, $v27.e5
[ 58] vmadh $v02, $v20, $v27.e5
[ 58] andi $t5, $a3, 1024
[ 59] vsubc $v29, $v08, $v08.e2
[ 59] lsv $v15, 10, 34, $a1
[ 60] lsv $v14, 8, 32, $a0
[ 60] vlt $v07, $v07, $v07.e2
[ 61] vmrg $v08, $v08, $v08.e2
[ 61] srl $t2, $t3, 13
[ 62] vmudl $v03, $v21, $v24.q1
[ 62] lsv $v14, 10, 32, $a1
[ 63] vmadm $v03, $v20, $v24.q1
[ 63] mfc2 $t0, $v24.e3
[ 64] vmadn $v03, $v21, $v28.v
[ 64] andi $t6, $a3, 512
[ 65] vmadh $v04, $v20, $v28.v
[ 65] lsv $v14, 12, 32, $a2
[ 66] vmadn $v03, $v12, $v27.e7
[ 66] mfc2 $t1, $v24.e0
[ 67] vmadh $v04, $v12, $v27.e7
[ 67] luv $v09, 0, 8, $a0
[ 68] vsubc $v29, $v08, $v08.e4
[ 68] lsv $v15, 8, 34, $a0
[ 69] vlt $v07, $v07, $v07.e4
[ 69] luv $v10, 0, 8, $a1
[ 70] vmrg $v08, $v08, $v08.e4
[ 70] addiu $at, $zero, 207
[ 71] vmudl $v29, $v03, $v01
[ 71] luv $v11, 0, 8, $a2
[ 72] vmadm $v29, $v04, $v01
[ 72] or $a3, $a3, $t2
[ 73] vmadn $v19, $v03, $v02.v
[ 73] srl $t5, $t5, 4
[ 74] sh $a3, 0($s3)
[ 74] vmadh $v18, $v04, $v02.v
[ 75] subu $t1, $zero, $t1
[ 75] vmudl $v29, $v15, $v08.e0
[ 76] vmadm $v29, $v14, $v08.e0
[ 76] ctc2 $at, $vcc
[ 77] vmadn $v15, $v15, $v07.e0
[ 77] llv $v03, 8, 12, $a1
[ 78] vmudn $v29, $v19, $v24.v
[ 78] llv $v06, 8, 12, $a2
[ 79] vmadh $v29, $v18, $v24.v
[ 79] subu $t0, $zero, $t0
[ 80] vsar $v20, COP2_ACC_HI
[ 80] andi $t3, $a3, 256
[ 81] vsar $v21, COP2_ACC_MD
[ 81] mtc2 $t1, $v24.e0
[ 82] mtc2 $t0, $v24.e3
[ 82] vsubc $v15, $v15, $v30.e7
[ 83] vmudl $v10, $v10, $v31.e6
[ 83] llv $v04, 8, 12, $a0
[ 84] addu $t5, $t5, $t4
[ 84] vmudl $v09, $v09, $v31.e6
[ 85] lbu $t0, %lo(RDPQ_SYNCFULL_ONGOING + 0)
[ 85] vmudl $v11, $v11, $v31.e6
[ 86] vmudl $v15, $v15, $v31.e0
[ 86] mfc0 $t1, COP0_DMA_BUSY
[ 87] vmudl $v29, $v21, $v26.e4
[ 87] lsv $v10, 14, 4, $a1
[ 88] vmadm $v29, $v20, $v26.e4
[ 88] srl $t6, $t6, 3
[ 89] vmadn $v17, $v21, $v25.e4
[ 89] lsv $v11, 14, 4, $a2
[ 90] sdv $v15, 8, 16, $s3
[ 90] vmadh $v16, $v20, $v25.e4
[ 91] lsv $v09, 14, 4, $a0
[ 91] vmudm $v22, $v23, $v31.e1
[ 92] lsv $v09, 12, 16, $s3
[ 92] vmulf $v06, $v06, $v15.h2
[ 93] lsv $v10, 12, 18, $s3
[ 93] vmulf $v04, $v04, $v15.h0
[ 94] lsv $v11, 12, 20, $s3
[ 94] vmulf $v03, $v03, $v15.h1
[ 95] vmudn $v23, $v23, $v31.e1
[ 95] bne $t1, $zero, LABEL_RDPQ_Triangle_Send_Async_0006  # unlikely
[ 96] addu $t6, $t6, $t5
[ 97] vmrg $v09, $v09, $v04
[ 97] lw $a0, %lo(RDPQ_CURRENT + 0)
[ 98] vmrg $v10, $v10, $v03
[ 98] ssv $v22, 4, 8, $s3
[ 99] srl $t3, $t3, 4
[ 99] vmrg $v11, $v11, $v06
[100] vaddc $v17, $v17, $v23.e0
[100] ssv $v23, 4, 10, $s3
[101] ssv $v20, 8, 12, $s3
[101] vadd $v16, $v16, $v22.e0
[102] bne $t0, $zero, LABEL_RDPQ_Triangle_Send_Async_0008  # unlikely
[103] ssv $v21, 8, 14, $s3
)";

TEST_CASE("Eval - Cost (Examples) - TRI RSPL2 seg0", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_RSPL2_SEG0);
  auto cyclesExp = textToAsmCycle(TRI_RSPL2_SEG0);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) + ": model=" + std::to_string(cycles[line]) + " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}

// ares-measured ground truth (00_quad, @Unlikely layout, 158-cycle schedule, segment 1: entry pc 0x568, 77 instr, 45 cycles)
static const std::string TRI_RSPL2_SEG1 = R"(
[  0] vsubc $v29, $v00, $v00
[  1] vsub $v12, $v10, $v09.v
[  1] lw $a2, %lo(RDPQ_SENTINEL + 0)
[  2] addu $t3, $t3, $t6
[  2] vsub $v13, $v11, $v09.v
[  3] subu $t3, $t3, $s3
[  3] vmudn $v19, $v19, $v30.e5
[  4] vmadh $v18, $v18, $v30.e5
[  4] ssv $v16, 0, 16, $s3
[  5] ssv $v17, 0, 18, $s3
[  5] vmudh $v29, $v12, $v24.e1
[  6] vmadh $v29, $v13, $v24.e3
[  7] vsar $v03, COP2_ACC_HI
[  8] vsar $v04, COP2_ACC_MD
[  8] sh $s7, %lo(RDPQ_TRI_BUFF_OFFSET)($zero)
[  9] vmudh $v29, $v13, $v24.e2
[  9] ssv $v20, 0, 20, $s3
[ 10] vmadh $v29, $v12, $v24.e0
[ 11] vsar $v07, COP2_ACC_HI
[ 11] ssv $v21, 0, 22, $s3
[ 12] ssv $v17, 4, 26, $s3
[ 12] vsar $v08, COP2_ACC_MD
[ 13] vmudl $v29, $v04, $v19.e7
[ 13] mtc0 $a0, COP0_DMA_RAMADDR
[ 14] ssv $v20, 4, 28, $s3
[ 14] vmadm $v29, $v03, $v19.e7
[ 15] vmadn $v04, $v04, $v18.e7
[ 15] addu $a0, $a0, $t3
[ 16] vmadh $v03, $v03, $v18.e7
[ 16] sltu $at, $a2, $a0
[ 17] vmudl $v29, $v08, $v19.e7
[ 17] ssv $v16, 4, 24, $s3
[ 18] mtc0 $s3, COP0_DMA_SPADDR
[ 18] vmadm $v29, $v07, $v19.e7
[ 19] ssv $v21, 4, 30, $s3
[ 19] vmadn $v08, $v08, $v18.e7
[ 20] vmadh $v07, $v07, $v18.e7
[ 20] addu $s3, $s3, $t3
[ 21] sdv $v03, 0, 8, $t4
[ 21] vmadl $v29, $v04, $v21.e0
[ 22] vmadm $v29, $v03, $v21.e0
[ 22] sdv $v04, 0, 24, $t4
[ 23] sdv $v04, 8, 24, $t5
[ 23] vmadn $v06, $v04, $v20.e0
[ 24] vmadh $v05, $v03, $v20.e0
[ 24] sdv $v03, 8, 8, $t5
[ 25] bne $at, $zero, LABEL_RDPQ_Triangle_Send_Async_0009  # unlikely
[ 26] sdv $v08, 0, 56, $t4
[ 27] vmudh $v29, $v09, $v30.e7
[ 27] addiu $t3, $t3, 65535
[ 28] vmadl $v29, $v06, $v26.e4
[ 28] sw $a0, %lo(RDPQ_CURRENT)($zero)
[ 29] vmadm $v29, $v05, $v26.e4
[ 29] sdv $v07, 0, 40, $t4
[ 30] vmadn $v02, $v06, $v25.e4
[ 30] sdv $v06, 0, 48, $t4
[ 31] sdv $v06, 8, 48, $t5
[ 31] vmadh $v01, $v05, $v25.e4
[ 32] vmov $v10.e2, $v03.e7
[ 32] sdv $v05, 8, 32, $t5
[ 33] vmov $v06.e6, $v05.e7
[ 33] sdv $v08, 8, 56, $t5
[ 34] sdv $v02, 0, 16, $t4
[ 34] vmov $v08.e6, $v07.e7
[ 35] vmov $v10.e0, $v01.e7
[ 35] sdv $v05, 0, 32, $t4
[ 36] vmov $v10.e1, $v02.e7
[ 36] sdv $v01, 0, 0, $t4
[ 37] vmov $v10.e3, $v04.e7
[ 37] slv $v06, 12, 8, $t6
[ 38] slv $v08, 12, 12, $t6
[ 39] sdv $v01, 8, 0, $t5
[ 40] sdv $v07, 8, 40, $t5
[ 41] sdv $v02, 8, 16, $t5
[ 42] sdv $v10, 0, 0, $t6
[ 43] jr $ra  # unlikely
[ 44] mtc0 $t3, COP0_DMA_WRITE
)";

TEST_CASE("Eval - Cost (Examples) - TRI RSPL2 seg1", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_RSPL2_SEG1);
  auto cyclesExp = textToAsmCycle(TRI_RSPL2_SEG1);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) + ": model=" + std::to_string(cycles[line]) + " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}


// ares-measured ground truth (00_quad, 156-cycle schedule after pairing fix, segment 0: entry pc 0x280, 185 instr, 102 cycles)
static const std::string TRI_RSPL3_SEG0 = R"(
[  0] lbu $v0, %lo(FACE_CULLING + 0)
[  1] srl $a2, $a1, 16
[  1] vsubc $v27, $v00, $v30.v
[  2] llv $v01, 0, 0, $a0
[  3] mtc2 $a1, $v02.e3
[  3] vaddc $v12, $v00, $v30.e7
[  4] llv $v02, 0, 0, $a1
[  5] lhu $t3, 6($a0)
[  6] llv $v03, 0, 0, $a2
[  7] addiu $at, $zero, 255
[  7] vor $v06, $v00, $v01.e1
[  8] lhu $t4, 6($a1)
[  8] vor $v07, $v00, $v02.e1
[  9] mtc2 $a0, $v01.e3
[ 10] lhu $t5, 6($a2)
[ 11] or $t3, $t3, $t4
[ 11] vor $v08, $v00, $v03.e1
[ 12] vge $v09, $v06, $v07
[ 12] lhu $s7, %lo(RDPQ_TRI_BUFF_OFFSET + 0)
[ 13] vmrg $v04, $v01, $v02
[ 13] mtc2 $a2, $v03.e3
[ 14] vlt $v06, $v06, $v07
[ 14] or $t3, $t3, $t5
[ 15] vmrg $v01, $v01, $v02
[ 15] andi $t6, $t3, 255
[ 16] vxor $v28, $v28, $v28.v
[ 16] bne $t6, $at, JrRa  # unlikely
[ 17] cfc2 $t0, $vcc
[ 18] andi $t4, $t3, 7936
[ 18] vge $v10, $v06, $v08
[ 19] vmrg $v05, $v01, $v03
[ 20] vlt $v06, $v06, $v08
[ 21] cfc2 $t1, $vcc
[ 21] vmrg $v01, $v01, $v03
[ 22] vge $v08, $v09, $v10
[ 22] addiu $s3, $s7, %lo(CLIP_BUFFER_TMP)
[ 23] lw $a3, %lo(TRI_COMMAND + 0)
[ 23] vmrg $v03, $v04, $v05
[ 24] vlt $v07, $v09, $v10
[ 24] ssv $v06, 0, 6, $s3
[ 25] vmrg $v02, $v04, $v05
[ 25] bne $t4, $zero, RDPQ_Triangle_Clip  # unlikely
[ 26] cfc2 $t2, $vcc
[ 27] vmov $v23.e0, $v01.e0
[ 27] xori $s7, $s7, 176
[ 28] mfc2 $a0, $v01.e3
[ 28] vsubc $v05, $v02, $v01.v
[ 30] vsubc $v04, $v03, $v02.v
[ 30] mfc2 $a1, $v02.e3
[ 31] vmudm $v25, $v06, $v31.e1
[ 31] xor $t0, $t0, $t1
[ 32] xor $t0, $t0, $t2
[ 32] vsubc $v24, $v03, $v01.v
[ 33] vsubc $v29, $v00, $v05.e1
[ 33] slv $v05, 0, 8, $s3
[ 34] andi $t0, $t0, 1
[ 34] vmov $v23.e2, $v02.e0
[ 35] vmudn $v26, $v06, $v31.e1
[ 35] slv $v04, 0, 12, $s3
[ 36] vmudh $v19, $v05, $v24.e1
[ 36] mfc2 $a2, $v03.e3
[ 37] vmadh $v19, $v24, $v29.v
[ 37] xor $t1, $v0, $t0
[ 38] ldv $v24, 4, 8, $s3
[ 38] vsar $v19, COP2_ACC_MD
[ 39] vsar $v18, COP2_ACC_HI
[ 39] ssv $v08, 0, 2, $s3
[ 40] vsubc $v26, $v00, $v26.v
[ 40] lsv $v08, 4, 30, $a1
[ 41] ssv $v07, 0, 4, $s3
[ 41] vsub $v25, $v25, $v25.v
[ 42] vmov $v24.e7, $v19.e0
[ 42] lsv $v08, 0, 30, $a0
[ 43] mfc2 $t0, $v18.e0
[ 43] vrcph $v20.e7, $v18.e0
[ 44] vrcpl $v21.e7, $v19.e0
[ 44] lsv $v08, 8, 30, $a2
[ 45] vrcph $v20.e7, $v00.e0
[ 45] addiu $t4, $s3, 32
[ 46] vmov $v28.e7, $v18.e0
[ 46] slt $t0, $t0, $zero
[ 47] vrcp $v21.e0, $v24.e1
[ 47] beq $t0, $t1, JrRa  # unlikely
[ 48] vrcph $v20.e0, $v24.e1
[ 49] vrcp $v21.e2, $v05.e1
[ 49] xori $t0, $t0, 1
[ 50] lsv $v07, 8, 22, $a2
[ 50] vrcph $v20.e2, $v05.e1
[ 51] vrcp $v21.e4, $v04.e1
[ 51] sll $t0, $t0, 7
[ 52] srl $t2, $t3, 13
[ 52] vrcph $v20.e4, $v04.e1
[ 53] lsv $v07, 0, 22, $a0
[ 54] lsv $v07, 4, 22, $a1
[ 55] lsv $v14, 8, 32, $a0
[ 55] vmudn $v01, $v21, $v27.e5
[ 56] vmadh $v02, $v20, $v27.e5
[ 56] luv $v11, 0, 8, $a2
[ 57] lsv $v14, 12, 32, $a2
[ 57] vsubc $v29, $v08, $v08.e2
[ 58] vlt $v07, $v07, $v07.e2
[ 58] llv $v06, 8, 12, $a2
[ 59] lsv $v15, 8, 34, $a0
[ 59] vmrg $v08, $v08, $v08.e2
[ 60] vmudl $v03, $v21, $v24.q1
[ 60] or $a3, $a3, $t0
[ 61] vmadm $v03, $v20, $v24.q1
[ 61] luv $v09, 0, 8, $a0
[ 62] vmadn $v03, $v21, $v28.v
[ 62] lsv $v14, 10, 32, $a1
[ 63] vmadh $v04, $v20, $v28.v
[ 63] addiu $at, $zero, 207
[ 64] vmadn $v03, $v12, $v27.e7
[ 64] andi $t6, $a3, 512
[ 65] vmadh $v04, $v12, $v27.e7
[ 65] mfc2 $t1, $v24.e0
[ 66] vsubc $v29, $v08, $v08.e4
[ 66] luv $v10, 0, 8, $a1
[ 67] lsv $v15, 12, 34, $a2
[ 67] vlt $v07, $v07, $v07.e4
[ 68] vmrg $v08, $v08, $v08.e4
[ 68] andi $t5, $a3, 1024
[ 69] vmudl $v29, $v03, $v01
[ 69] lsv $v15, 10, 34, $a1
[ 70] vmadm $v29, $v04, $v01
[ 70] ctc2 $at, $vcc
[ 71] vmadn $v19, $v03, $v02.v
[ 71] subu $t1, $zero, $t1
[ 72] vmadh $v18, $v04, $v02.v
[ 72] srl $t5, $t5, 4
[ 73] vmudl $v29, $v15, $v08.e0
[ 73] llv $v04, 8, 12, $a0
[ 74] vmadm $v29, $v14, $v08.e0
[ 74] mfc2 $t0, $v24.e3
[ 75] vmadn $v15, $v15, $v07.e0
[ 75] addu $t5, $t5, $t4
[ 76] vmudn $v29, $v19, $v24.v
[ 76] or $a3, $a3, $t2
[ 77] vmadh $v29, $v18, $v24.v
[ 77] sh $a3, 0($s3)
[ 78] mtc2 $t1, $v24.e0
[ 78] vsar $v21, COP2_ACC_MD
[ 79] vsar $v20, COP2_ACC_HI
[ 79] subu $t0, $zero, $t0
[ 80] vsubc $v15, $v15, $v30.e7
[ 80] llv $v03, 8, 12, $a1
[ 81] vmudl $v09, $v09, $v31.e6
[ 81] mtc2 $t0, $v24.e3
[ 82] vmudl $v11, $v11, $v31.e6
[ 82] lbu $t0, %lo(RDPQ_SYNCFULL_ONGOING + 0)
[ 83] lsv $v11, 14, 4, $a2
[ 83] vmudl $v10, $v10, $v31.e6
[ 84] vmudl $v15, $v15, $v31.e0
[ 84] lsv $v09, 14, 4, $a0
[ 85] vmudl $v29, $v21, $v26.e4
[ 85] lsv $v10, 14, 4, $a1
[ 86] vmadm $v29, $v20, $v26.e4
[ 86] srl $t6, $t6, 3
[ 87] addu $t6, $t6, $t5
[ 87] vmadn $v17, $v21, $v25.e4
[ 88] vmadh $v16, $v20, $v25.e4
[ 88] sdv $v15, 8, 16, $s3
[ 89] mfc0 $t1, COP0_DMA_BUSY
[ 89] vmulf $v06, $v06, $v15.h2
[ 90] lsv $v09, 12, 16, $s3
[ 90] vmulf $v03, $v03, $v15.h1
[ 91] lsv $v10, 12, 18, $s3
[ 91] vmulf $v04, $v04, $v15.h0
[ 92] vmudm $v22, $v23, $v31.e1
[ 92] lsv $v11, 12, 20, $s3
[ 93] vmudn $v23, $v23, $v31.e1
[ 93] bne $t1, $zero, LABEL_RDPQ_Triangle_Send_Async_0006  # unlikely
[ 94] andi $t3, $a3, 256
[ 95] vmrg $v09, $v09, $v04
[ 95] lw $a0, %lo(RDPQ_CURRENT + 0)
[ 96] ssv $v22, 4, 8, $s3
[ 96] vmrg $v10, $v10, $v03
[ 97] vmrg $v11, $v11, $v06
[ 97] srl $t3, $t3, 4
[ 98] ssv $v23, 4, 10, $s3
[ 98] vaddc $v17, $v17, $v23.e0
[ 99] vadd $v16, $v16, $v22.e0
[ 99] ssv $v20, 8, 12, $s3
[100] bne $t0, $zero, LABEL_RDPQ_Triangle_Send_Async_0008  # unlikely
[101] ssv $v21, 8, 14, $s3
)";

TEST_CASE("Eval - Cost (Examples) - TRI RSPL3 seg0", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_RSPL3_SEG0);
  auto cyclesExp = textToAsmCycle(TRI_RSPL3_SEG0);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) + ": model=" + std::to_string(cycles[line]) + " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}

// ares-measured ground truth (00_quad, 156-cycle schedule after pairing fix, segment 1: entry pc 0x568, 77 instr, 45 cycles)
static const std::string TRI_RSPL3_SEG1 = R"(
[  0] vsubc $v29, $v00, $v00
[  0] ssv $v16, 0, 16, $s3
[  1] vsub $v12, $v10, $v09.v
[  2] vsub $v13, $v11, $v09.v
[  3] vmudn $v19, $v19, $v30.e5
[  3] lw $a2, %lo(RDPQ_SENTINEL + 0)
[  4] mtc0 $a0, COP0_DMA_RAMADDR
[  4] vmadh $v18, $v18, $v30.e5
[  5] addu $t3, $t3, $t6
[  5] vmudh $v29, $v12, $v24.e1
[  6] vmadh $v29, $v13, $v24.e3
[  7] vsar $v03, COP2_ACC_HI
[  7] ssv $v17, 0, 18, $s3
[  8] vsar $v04, COP2_ACC_MD
[  8] subu $t3, $t3, $s3
[  9] ssv $v20, 4, 28, $s3
[  9] vmudh $v29, $v13, $v24.e2
[ 10] vmadh $v29, $v12, $v24.e0
[ 10] sh $s7, %lo(RDPQ_TRI_BUFF_OFFSET)($zero)
[ 11] vsar $v07, COP2_ACC_HI
[ 11] addu $a0, $a0, $t3
[ 12] vsar $v08, COP2_ACC_MD
[ 13] vmudl $v29, $v04, $v19.e7
[ 13] mtc0 $s3, COP0_DMA_SPADDR
[ 14] ssv $v16, 4, 24, $s3
[ 14] vmadm $v29, $v03, $v19.e7
[ 15] vmadn $v04, $v04, $v18.e7
[ 15] sltu $at, $a2, $a0
[ 16] vmadh $v03, $v03, $v18.e7
[ 16] ssv $v20, 0, 20, $s3
[ 17] vmudl $v29, $v08, $v19.e7
[ 17] ssv $v17, 4, 26, $s3
[ 18] ssv $v21, 4, 30, $s3
[ 18] vmadm $v29, $v07, $v19.e7
[ 19] ssv $v21, 0, 22, $s3
[ 19] vmadn $v08, $v08, $v18.e7
[ 20] sdv $v03, 8, 8, $t5
[ 20] vmadh $v07, $v07, $v18.e7
[ 21] addu $s3, $s3, $t3
[ 21] vmadl $v29, $v04, $v21.e0
[ 22] vmadm $v29, $v03, $v21.e0
[ 22] sdv $v03, 0, 8, $t4
[ 23] vmadn $v06, $v04, $v20.e0
[ 23] sdv $v08, 0, 56, $t4
[ 24] sdv $v04, 0, 24, $t4
[ 24] vmadh $v05, $v03, $v20.e0
[ 25] bne $at, $zero, LABEL_RDPQ_Triangle_Send_Async_0009  # unlikely
[ 26] sdv $v04, 8, 24, $t5
[ 27] vmudh $v29, $v09, $v30.e7
[ 27] sdv $v08, 8, 56, $t5
[ 28] vmadl $v29, $v06, $v26.e4
[ 28] sw $a0, %lo(RDPQ_CURRENT)($zero)
[ 29] vmadm $v29, $v05, $v26.e4
[ 29] sdv $v05, 8, 32, $t5
[ 30] vmadn $v02, $v06, $v25.e4
[ 30] addiu $t3, $t3, 65535
[ 31] vmadh $v01, $v05, $v25.e4
[ 31] sdv $v06, 8, 48, $t5
[ 32] vmov $v10.e3, $v04.e7
[ 32] sdv $v07, 0, 40, $t4
[ 33] vmov $v10.e2, $v03.e7
[ 33] sdv $v05, 0, 32, $t4
[ 34] vmov $v08.e6, $v07.e7
[ 34] sdv $v06, 0, 48, $t4
[ 35] sdv $v02, 8, 16, $t5
[ 35] vmov $v06.e6, $v05.e7
[ 36] sdv $v01, 0, 0, $t4
[ 36] vmov $v10.e0, $v01.e7
[ 37] sdv $v01, 8, 0, $t5
[ 37] vmov $v10.e1, $v02.e7
[ 38] slv $v08, 12, 12, $t6
[ 39] slv $v06, 12, 8, $t6
[ 40] sdv $v02, 0, 16, $t4
[ 41] sdv $v07, 8, 40, $t5
[ 42] sdv $v10, 0, 0, $t6
[ 43] jr $ra  # unlikely
[ 44] mtc0 $t3, COP0_DMA_WRITE
)";

TEST_CASE("Eval - Cost (Examples) - TRI RSPL3 seg1", "[evalCostExample]") {
  auto lines = textToAsmLines(TRI_RSPL3_SEG1);
  auto cyclesExp = textToAsmCycle(TRI_RSPL3_SEG1);
  auto cycles = linesToCycles(lines);
  REQUIRE(cycles.size() == cyclesExp.size());
  std::string report;
  int run = 0;
  for (size_t line = 0; line < cycles.size(); ++line) {
    int d = cycles[line] - cyclesExp[line];
    run = (d != 0) ? run + 1 : 0;
    if (run > 2)
      report += "persistent divergence at line " + std::to_string(line) + ": model=" + std::to_string(cycles[line]) + " real=" + std::to_string(cyclesExp[line]) + "\n";
  }
  INFO("model-vs-ares:\n" << report);
  REQUIRE(report.empty());
  REQUIRE(cycles.back() == cyclesExp.back());
}

// Triage helper (hidden): prints model vs ares cycles side by side.
// Run with: rspl_tests "[.triage]"
#include <iostream>
static void dumpWindow(const std::string &text, const char *name, int from, int to) {
  auto lines = textToAsmLines(text);
  auto cyclesExp = textToAsmCycle(text);
  auto cycles = linesToCycles(lines);
  std::istringstream ss(text);
  std::string l; std::vector<std::string> raw;
  while (std::getline(ss, l)) { if (l.find(']') != std::string::npos) raw.push_back(l); }
  std::cout << "=== " << name << " lines " << from << ".." << to << " (model | real)\n";
  for (int i = from; i <= to && i < (int)cycles.size(); ++i)
    std::cout << (cycles[i] == cyclesExp[i] ? "  " : "! ")
              << std::to_string(cycles[i]) << "\t" << cyclesExp[i] << "\t" << raw[i] << "\n";
}
TEST_CASE("Eval - triage dump", "[.triage]") {
  dumpWindow(TRI_RSPL_SEG0, "RSPL seg0", 38, 50);
  dumpWindow(TRI_RSPL_SEG3, "RSPL seg3", 0, 6);
  dumpWindow(TRI_REF_SEG0, "REF seg0", 0, 22);
  dumpWindow(TRI_RSPL2_SEG0, "RSPL2 seg0", 30, 52);
}

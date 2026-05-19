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

#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "optimizer/asm_scan_deps.h"
#include "optimizer/eval_cost.h"

#include <sstream>
#include <string>
#include <vector>

using namespace rspl;

// Parse text lines like "or $t0, $zero, $zero" into AsmInst vectors
static std::vector<AsmInst> textToAsmLines(const std::string &text) {
  std::vector<AsmInst> lines;
  std::istringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    // trim
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    size_t end = line.find_last_not_of(" \t");
    line = line.substr(start, end - start + 1);
    if (line.empty()) continue;

    std::istringstream ls(line);
    std::string op;
    ls >> op;
    std::vector<std::string> args;
    std::string arg;
    while (ls >> arg) {
      if (arg.back() == ',') arg.pop_back();
      args.push_back(arg);
    }
    if (op == "nop")
      lines.push_back(asmNOP());
    else
      lines.push_back(asmOp(op, args));
  }
  return lines;
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

#define CHECK_CYCLES(name, text, ...)                            \
  TEST_CASE("Eval - Cost - " name, "[evalCost]") {               \
    auto lines = textToAsmLines(text);                           \
    std::vector<int> expected = __VA_ARGS__;                     \
    auto cycles = linesToCycles(lines);                          \
    REQUIRE(cycles == expected);                                 \
  }

CHECK_CYCLES("SU only - no dep",
  "or $t0, $zero, $zero\n"
  "addiu $t0, $t0, 1\n"
  "addiu $t0, $t0, 1\n"
  "addiu $t0, $t0, 1\n"
  "jr $ra\n"
  "nop\n",
  {1, 2, 3, 4, 5, 7})

CHECK_CYCLES("SU only - deps",
  "or $t0, $zero, $zero\n"
  "or $t1, $zero, $t0\n"
  "addu $t2, $t0, $t1\n",
  {1, 2, 3})

CHECK_CYCLES("VU only - no dep",
  "vxor $v01, $v00, $v00.e0\n"
  "vxor $v01, $v00, $v30.e7\n"
  "vxor $v01, $v00, $v30.e6\n"
  "vxor $v01, $v00, $v30.e5\n",
  {1, 2, 3, 4})

CHECK_CYCLES("VU only - ACC / 32-bit mul",
  "vmudl $v29, $v05, $v09.v\n"
  "vmadm $v29, $v04, $v09.v\n"
  "vmadn $v11, $v05, $v08.v\n"
  "vmadh $v10, $v04, $v08.v\n",
  {1, 2, 3, 4})

CHECK_CYCLES("VU only - DIV / invert_half",
  "vrcph $v04.e0, $v04.e0\n"
  "vrcpl $v05.e0, $v05.e0\n"
  "vrcph $v04.e0, $v08.e0\n"
  "vrcpl $v09.e0, $v09.e0\n"
  "vrcph $v08.e0, $v00.e0\n",
  {1, 2, 3, 4, 5})

CHECK_CYCLES("VU only - ternary",
  "vaddc $v06, $v06, $v06.v\n"
  "vadd $v11, $v05, $v05.v\n"
  "vne $v29, $v18, $v00.e0\n"
  "vmrg $v13, $v05, $v07\n"
  "vmrg $v14, $v06, $v08\n",
  {1, 2, 3, 4, 5})

CHECK_CYCLES("VU only - deps",
  "vxor $v01, $v00, $v00.e0\n"
  "vaddc $v04, $v01, $v01.v\n"
  "vxor $v05, $v00, $v00.e0\n"
  "vaddc $v04, $v01, $v01.v\n",
  {1, 5, 6, 7})

CHECK_CYCLES("SU/VU mix - no dep",
  "vxor $v01, $v00, $v00.e0\n"
  "vxor $v01, $v00, $v30.e7\n"
  "addiu $t0, $zero, 4\n"
  "vxor $v01, $v00, $v30.e6\n"
  "addiu $t1, $zero, 4\n"
  "addiu $t2, $zero, 4\n",
  {1, 2, 2, 3, 3, 4})

CHECK_CYCLES("VU - same src/dst dep",
  "vxor $v01, $v00, $v30.e7\n"
  "vxor $v01, $v01, $v01\n"
  "vor $v05, $v00, $v01\n",
  {1, 5, 9})

CHECK_CYCLES("VU/SU - no dual issue (lqv)",
  "vand $v04, $v30, $v31\n"
  "lqv $v04, 0, 0, $s4\n",
  {1, 2})

CHECK_CYCLES("VU/SU - no dual issue (mfc2)",
  "vand $v04, $v30, $v31\n"
  "mfc2 $t0, $v04.e4\n",
  {1, 5})

CHECK_CYCLES("SU/VU mix - delay slot",
  "or $t0, $zero, $zero\n"
  "bne $t0, $zero, END\n"
  "nop\n"
  "vxor $v01, $v00, $v30.e7\n"
  "vaddc $v02, $v03, $v30.e7\n",
  {1, 2, 4, 5, 6})

CHECK_CYCLES("SU/VU mix - MTC2",
  "vxor $v04, $v00, $v00.e0\n"
  "vxor $v05, $v00, $v00\n"
  "mtc2 $t0, $v05.e0\n"
  "srl $at, $t0, 16\n"
  "srl $at, $t0, 16\n"
  "mtc2 $at, $v04.e0\n"
  "vmov $v04.e2, $v04.e0\n"
  "vmov $v05.e2, $v05.e0\n"
  "vmov $v04.e3, $v04.e1\n"
  "vmov $v05.e3, $v05.e1\n",
  {1, 2, 3, 4, 5, 6, 10, 11, 14, 15})

CHECK_CYCLES("SU/VU in dual - MFC2",
  "vmadn $v06, $v06, $v05.h3\n"
  "vmadh $v05, $v05, $v05.h3\n"
  "nop\n"
  "mfc2 $fp, $v02.e2\n"
  "vmudl $v29, $v06, $v14.v\n"
  "vor $v00, $v00, $v00\n"
  "nop\n"
  "sra $fp, $fp, 7\n",
  {1, 2, 2, 5, 5, 6, 6, 8})

CHECK_CYCLES("SU memory - load (dep)",
  "lhu $s3, 24($s4)\n"
  "lhu $s2, 24($s4)\n"
  "srl $s2, $s2, 2\n",
  {1, 2, 5})

CHECK_CYCLES("SU memory - load/store (dep)",
  "lw $t1, 0($t0)\n"
  "addiu $t2, $zero, 3\n"
  "sw $t1, ($t0)\n"
  "addiu $t2, $zero, 4\n",
  {1, 2, 4, 5})

CHECK_CYCLES("SU memory - load/store (no-dep)",
  "lw $t4, 0($t0)\n"
  "addiu $t2, $zero, 3\n"
  "sw $t1, ($t0)\n"
  "addiu $t2, $zero, 4\n",
  {1, 2, 4, 5})

CHECK_CYCLES("SU memory - load/store (multiple)",
  "lhu $s3, 24($s4)\n"
  "lhu $s2, 24($s4)\n"
  "lhu $s1, 24($s4)\n"
  "sh $s3, 12($s6)\n"
  "sh $s2, 12($s5)\n"
  "sh $s1, 12($s5)\n",
  {1, 2, 3, 6, 7, 8})

CHECK_CYCLES("SU memory - load/store (no-dep, dual)",
  "lw $t4, 0($t0)\n"
  "vxor $v11, $v11, $v11\n"
  "addiu $t2, $zero, 3\n"
  "sw $t1, ($t0)\n"
  "addiu $t2, $zero, 4\n",
  {1, 1, 2, 4, 5})

CHECK_CYCLES("CFC2 - stall",
  "cfc2 $sp, $vcc\n"
  "andi $sp, $sp, 1799\n"
  "srl $t7, $sp, 5\n",
  {1, 4, 5})

CHECK_CYCLES("VU + CFC2 - dual",
  "vxor $v01, $v01, $v01\n"
  "cfc2 $sp, $vcc\n"
  "andi $sp, $sp, 1799\n"
  "srl $t7, $sp, 5\n",
  {1, 1, 4, 5})

CHECK_CYCLES("CFC2 + VU - dual",
  "cfc2 $sp, $vcc\n"
  "vxor $v01, $v01, $v01\n"
  "andi $sp, $sp, 1799\n"
  "srl $t7, $sp, 5\n",
  {1, 1, 4, 5})

CHECK_CYCLES("VU + CFC2 - no-dual",
  "vcl $v29, $v27, $v20\n"
  "cfc2 $sp, $vcc\n"
  "andi $sp, $sp, 1799\n"
  "srl $t7, $sp, 5\n",
  {1, 2, 5, 6})

CHECK_CYCLES("CFC2 + VU - dual 2",
  "cfc2 $sp, $vcc\n"
  "vcl $v29, $v27, $v20\n"
  "andi $sp, $sp, 1799\n"
  "srl $t7, $sp, 5\n",
  {1, 1, 4, 5})

CHECK_CYCLES("Branch - NOP",
  "or $s7, $zero, $zero\n"
  "beq $s7, $zero, LABEL_0001\n"
  "nop\n"
  "vxor $v28, $v00, $v30.e7\n",
  {1, 2, 4, 5})

CHECK_CYCLES("Branch - dual-issue",
  "vxor $v00, $v00, $v00\n"
  "bne $s7, $zero, LABEL_0001\n"
  "nop\n"
  "vxor $v28, $v00, $v30.e7\n"
  "nop\n",
  {1, 1, 3, 5, 5})

CHECK_CYCLES("Branch multiple - NOP",
  "beq $zero, $zero, LABEL_A\n"
  "nop\n"
  "beq $zero, $zero, LABEL_B\n"
  "nop\n",
  {1, 3, 4, 6})

CHECK_CYCLES("Branch - filled (scalar)",
  "or $s7, $zero, $zero\n"
  "beq $s7, $zero, LABEL_0001\n"
  "addiu $s6, $zero, 3\n"
  "addiu $s6, $zero, 1\n",
  {1, 2, 4, 5})

CHECK_CYCLES("Branch - filled + stall (scalar)",
  "lw $a0, %lo(SCREEN_SIZE_VEC + 0)\n"
  "beq $zero, $zero, LABEL_A\n"
  "addiu $a0, $a0, 1\n",
  {1, 2, 5})

CHECK_CYCLES("Branch - filled (vector)",
  "or $s7, $zero, $zero\n"
  "beq $s7, $zero, LABEL_0001\n"
  "vxor $v28, $v00, $v30.e7\n"
  "vxor $v28, $v00, $v30.e7\n",
  {1, 2, 4, 5})

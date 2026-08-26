#include <catch2/catch_test_macros.hpp>
#include "asm.h"
#include "optimizer/asm_scan_deps.h"
#include "optimizer/eval_cost.h"

#include <sstream>
#include <string>
#include <vector>

using namespace rspl;

// Path-aware objective: the cost is the weighted hot-path cycles (fall-through
// spine, W_HOT=64 per cycle, x8 per loop level) plus cold/alternative code at
// low weight (16 for skipped arms before the exit, 1 for code after it).

static constexpr int W_HOT = 64;
static constexpr int W_ALT = 16;
static constexpr int W_COLD = 1;

// Parses text lines; "name:" lines become labels, everything else an op.
static AsmFunc textToFunc(const std::string &text) {
  AsmFunc func;
  std::istringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    size_t end = line.find_last_not_of(" \t");
    line = line.substr(start, end - start + 1);
    if (line.empty()) continue;
    if (line.back() == ':') {
      func.asm_.push_back(asmLabel(line.substr(0, line.size() - 1)));
      continue;
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
    func.asm_.push_back(op == "nop" ? asmNOP() : asmOp(op, args));
  }
  asmInitDeps(func);
  return func;
}

static int costOf(const std::string &text, int *hot = nullptr) {
  AsmFunc f = textToFunc(text);
  int c = evalFunctionCost(f);
  if (hot) *hot = f.hotCycles;
  return c;
}

TEST_CASE("Eval - Path - straight line is all hot", "[evalCostPath]") {
  int hot = 0;
  int cost = costOf(
      "or $t0, $zero, $zero\n"
      "addiu $t0, $t0, 1\n"
      "addiu $t0, $t0, 1\n"
      "jr $ra\n"
      "nop\n", &hot);
  REQUIRE(hot == 6);
  REQUIRE(cost == 6 * W_HOT);
}

TEST_CASE("Eval - Path - cold block after jr costs almost nothing",
          "[evalCostPath]") {
  // An @Unlikely-style block parked after the function tail.
  int hotA = 0, hotB = 0;
  int base = costOf(
      "or $t0, $zero, $zero\n"
      "bne $t0, $zero, COLD\n"
      "nop\n"
      "JOIN:\n"
      "addiu $t0, $t0, 2\n"
      "jr $ra\n"
      "nop\n", &hotA);
  int withCold = costOf(
      "or $t0, $zero, $zero\n"
      "bne $t0, $zero, COLD\n"
      "nop\n"
      "JOIN:\n"
      "addiu $t0, $t0, 2\n"
      "jr $ra\n"
      "nop\n"
      "COLD:\n"
      "addiu $t0, $t0, 1\n"
      "addiu $t0, $t0, 1\n"
      "addiu $t0, $t0, 1\n"
      "j JOIN\n"
      "nop\n", &hotB);
  REQUIRE(hotA == hotB);          // hot path unchanged by the cold block
  REQUIRE(withCold > base);       // ...but it is not free
  REQUIRE(withCold - base < W_HOT); // ...and far below one hot cycle
  REQUIRE((withCold - base) % W_COLD == 0);
}

TEST_CASE("Eval - Path - forward jump skips an alternative arm",
          "[evalCostPath]") {
  // if/else lowering: the if-arm falls through, `beq $zero,$zero,END`
  // jumps over the else-arm, which is costed as an alternative (W_ALT).
  int hot = 0;
  int cost = costOf(
      "or $t0, $zero, $zero\n"
      "bne $t0, $zero, ELSE\n"
      "nop\n"
      "addiu $t1, $zero, 1\n"
      "beq $zero, $zero, END\n"
      "nop\n"
      "ELSE:\n"
      "addiu $t1, $zero, 2\n"
      "addiu $t1, $t1, 2\n"
      "addiu $t1, $t1, 2\n"
      "addiu $t1, $t1, 2\n"
      "END:\n"
      "jr $ra\n"
      "nop\n", &hot);
  // hot: or, bne, nop, addiu, beq, nop(+bubble), jr, nop = walked ops only
  REQUIRE(hot < 12);
  // else-arm (4 ops, 4 cycles) is charged at W_ALT
  REQUIRE(cost == hot * W_HOT + 4 * W_ALT);
}

TEST_CASE("Eval - Path - loop body is weighted up", "[evalCostPath]") {
  int hotLoop = 0, hotFlat = 0;
  int loopCost = costOf(
      "or $t0, $zero, $zero\n"
      "LOOP:\n"
      "addiu $t0, $t0, 1\n"
      "addiu $t1, $t1, 1\n"
      "bne $t0, $t2, LOOP\n"
      "nop\n"
      "jr $ra\n"
      "nop\n", &hotLoop);
  int flatCost = costOf(
      "or $t0, $zero, $zero\n"
      "addiu $t0, $t0, 1\n"
      "addiu $t1, $t1, 1\n"
      "bne $t0, $t2, ELSEWHERE\n"
      "nop\n"
      "jr $ra\n"
      "nop\n", &hotFlat);
  REQUIRE(hotLoop == hotFlat);   // same instructions, same hot cycles
  REQUIRE(loopCost > flatCost);  // ...but the loop body counts 8x
}

TEST_CASE("Eval - Path - all hot ops get a cycle, cold ops restart at 1",
          "[evalCostPath]") {
  AsmFunc f = textToFunc(
      "or $t0, $zero, $zero\n"
      "jr $ra\n"
      "nop\n"
      "COLD:\n"
      "addiu $t0, $t0, 1\n"
      "addiu $t0, $t0, 1\n");
  evalFunctionCost(f);
  std::vector<int> cycles;
  for (auto &inst : f.asm_)
    if (inst.type == AsmType::OP) cycles.push_back(inst.debug.cycle);
  REQUIRE(cycles == std::vector<int>{1, 2, 4, 1, 2});
}

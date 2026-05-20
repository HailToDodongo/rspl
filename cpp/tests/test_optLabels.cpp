#include <catch2/catch_test_macros.hpp>
#include "optimizer/patterns/dedupeLabels.h"
#include "asm.h"
#include "pipeline.h"

static rspl::TranspileResult optTranspile(const std::string &src) {
  return rspl::transpileSource(src, {.rspqWrapper = false, .optimize = true});
}

// Helpers for direct dedupeLabels unit tests
static rspl::AsmInst L(const std::string &name) {
  rspl::AsmInst inst;
  inst.type = rspl::AsmType::LABEL;
  inst.label = name;
  return inst;
}
static rspl::AsmInst O(const std::string &op,
                       std::vector<std::string> args = {}) {
  rspl::AsmInst inst;
  inst.type = rspl::AsmType::OP;
  inst.op = op;
  inst.args = std::move(args);
  return inst;
}
static rspl::AsmInst B(const std::string &op,
                       std::vector<std::string> args,
                       const std::string &labelEnd) {
  rspl::AsmInst inst = O(op, std::move(args));
  inst.labelEnd = labelEnd;
  return inst;
}

TEST_CASE("Optimizer E2E - Labels - De-dupe Labels", "[optLabels]") {
  auto res = optTranspile(R"(function test(u32 dummy)
{
  LABEL_A:
  LABEL_B:
  LABEL_C:
  goto LABEL_A;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  LABEL_C:
  j LABEL_C
  nop)");
}

TEST_CASE("Optimizer - dedupeLabels - consecutive labels deduped to last",
          "[optLabels]") {
  rspl::AsmFunc func;
  func.asm_ = {B("j", {"LABEL_A"}, "LABEL_A"), O("nop"), L("LABEL_A"),
               L("LABEL_B"), O("addiu", {"$t0", "$zero", "1"})};
  rspl::dedupeLabels(func);
  REQUIRE(func.asm_.size() == 4);
  REQUIRE(func.asm_[0].args[0] == "LABEL_B");
  REQUIRE(func.asm_[0].labelEnd == "LABEL_B");
  REQUIRE(func.asm_[2].label == "LABEL_B");
}

TEST_CASE("Optimizer - dedupeLabels - __ labels are never deduplicated",
          "[optLabels]") {
  rspl::AsmFunc func;
  func.asm_ = {B("j", {"SKIP"}, "SKIP"), O("nop"), L("__A"), L("__A"),
               L("__A"), O("addiu", {"$t0", "$zero", "1"})};
  rspl::dedupeLabels(func);
  int labelCount = 0;
  for (auto &inst : func.asm_)
    if (inst.type == rspl::AsmType::LABEL) ++labelCount;
  REQUIRE(labelCount == 3);
}

TEST_CASE("Optimizer - dedupeLabels - __ label breaks dedup chain",
          "[optLabels]") {
  rspl::AsmFunc func;
  func.asm_ = {B("j", {"SKIP"}, "SKIP"), O("nop"), L("SKIP"), L("__B"),
               L("SKIP"), O("addiu", {"$t0", "$zero", "1"})};
  rspl::dedupeLabels(func);
  bool hasDunderB = false;
  for (auto &inst : func.asm_)
    if (inst.label == "__B") hasDunderB = true;
  REQUIRE(hasDunderB);
}

TEST_CASE("Optimizer E2E - Labels - De-dupe Labels - keep single", "[optLabels]") {
  auto res = optTranspile(R"(function test(u32 dummy)
{
  LABEL_A:
  dummy += 1;
  LABEL_B:
  dummy += 2;
  LABEL_C:
  goto LABEL_A;
})");
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_ == R"(test:
  LABEL_A:
  addiu $a0, $a0, 1
  LABEL_B:
  addiu $a0, $a0, 2
  LABEL_C:
  j LABEL_A
  nop)");
}

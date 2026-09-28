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
  inst.cold->label = name;
  return inst;
}
static rspl::AsmInst O(const std::string &op,
                       std::vector<std::string> args = {}) {
  rspl::AsmInst inst;
  inst.type = rspl::AsmType::OP;
  inst.op = rspl::getOpcode(op);
  inst.args = std::move(args);
  return inst;
}
static rspl::AsmInst B(const std::string &op,
                       std::vector<std::string> args,
                       const std::string &labelEnd) {
  rspl::AsmInst inst = O(op, std::move(args));
  inst.cold->labelEnd = labelEnd;
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
  // user labels are never dropped or renamed (they may be referenced from
  // other functions or hand-written assembly), labels are free anyway
  REQUIRE(res.asm_ == R"(test:
  LABEL_A:
  LABEL_B:
  LABEL_C:
  j LABEL_A
  nop)");
}

TEST_CASE("Optimizer - dedupeLabels - consecutive generated labels deduped to last",
          "[optLabels]") {
  rspl::AsmFunc func;
  func.name = "test";
  func.asm_ = {B("j", {"LABEL_test_0001"}, "LABEL_test_0001"), O("nop"),
               L("LABEL_test_0001"), L("LABEL_test_0002"),
               O("addiu", {"$t0", "$zero", "1"})};
  rspl::dedupeLabels(func);
  REQUIRE(func.asm_.size() == 4);
  REQUIRE(func.asm_[0].args[0] == "LABEL_test_0002");
  REQUIRE(func.asm_[0].cold->labelEnd == "LABEL_test_0002");
  REQUIRE(func.asm_[2].cold->label == "LABEL_test_0002");
}

TEST_CASE("Optimizer - dedupeLabels - a generated label folds into the user label, either order",
          "[optLabels]") {
  for (bool generatedFirst : {true, false}) {
    rspl::AsmFunc func;
    func.name = "test";
    func.asm_ = {B("j", {"LABEL_test_0001"}, "LABEL_test_0001"), O("nop"),
                 L(generatedFirst ? "LABEL_test_0001" : "USER"),
                 L(generatedFirst ? "USER" : "LABEL_test_0001"),
                 O("addiu", {"$t0", "$zero", "1"})};
    rspl::dedupeLabels(func);
    REQUIRE(func.asm_.size() == 4);
    REQUIRE(func.asm_[0].args[0] == "USER");
    REQUIRE(func.asm_[0].cold->labelEnd == "USER");
    REQUIRE(func.asm_[2].cold->label == "USER");
  }
}

TEST_CASE("Optimizer - dedupeLabels - two user labels are both kept",
          "[optLabels]") {
  rspl::AsmFunc func;
  func.name = "test";
  func.asm_ = {B("j", {"USER_A"}, "USER_A"), O("nop"), L("USER_A"),
               L("USER_B"), O("addiu", {"$t0", "$zero", "1"})};
  rspl::dedupeLabels(func);
  REQUIRE(func.asm_.size() == 5);
  REQUIRE(func.asm_[0].args[0] == "USER_A");
  REQUIRE(func.asm_[2].cold->label == "USER_A");
  REQUIRE(func.asm_[3].cold->label == "USER_B");
}

TEST_CASE("Optimizer - isGeneratedLabel", "[optLabels]") {
  rspl::AsmFunc func;
  func.name = "test";
  REQUIRE(rspl::isGeneratedLabel(func, "LABEL_test_0001"));
  REQUIRE(rspl::isGeneratedLabel(func, "LABEL_test_00FF"));
  REQUIRE_FALSE(rspl::isGeneratedLabel(func, "LABEL_A"));
  REQUIRE_FALSE(rspl::isGeneratedLabel(func, "LABEL_other_0001"));
  REQUIRE_FALSE(rspl::isGeneratedLabel(func, "LABEL_test_001"));
  REQUIRE_FALSE(rspl::isGeneratedLabel(func, "LABEL_test_00G1"));
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
    if (inst.cold->label == "__B") hasDunderB = true;
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

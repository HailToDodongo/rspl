#include <catch2/catch_test_macros.hpp>
#include "state.h"
#include "registers.h"
#include "types.h"

using namespace rspl;

TEST_CASE("state basic lifecycle", "[state]") {
  state.reset();
  state.enterFunction("test", "function", 0);

  REQUIRE(state.func == "test");
  REQUIRE(state.funcType == "function");
  REQUIRE(state.varExists("ZERO"));
  REQUIRE(state.varExists("VZERO"));
  REQUIRE(state.varExists("RA"));

  state.leaveFunction();
  REQUIRE(state.func.empty());
}

TEST_CASE("state register allocation scalar", "[state]") {
  state.reset();
  state.enterFunction("test", "function", 0);

  std::string reg = state.allocRegisters("u32").reg;
  REQUIRE(!reg.empty());
  REQUIRE(!reg::isVecReg(reg)); // Scalar type gets scalar register

  state.declareVar("a", "u32", reg);
  REQUIRE(state.varExists("a"));

  // Register is marked used; next allocation gets a different one
  std::string reg2 = state.allocRegisters("u32").reg;
  REQUIRE(reg2 != reg);

  state.leaveFunction();
}

TEST_CASE("state register allocation vector", "[state]") {
  state.reset();
  state.enterFunction("test", "function", 0);

  std::string reg = state.allocRegisters("vec16").reg;
  REQUIRE(!reg.empty());
  REQUIRE(reg::isVecReg(reg));

  state.leaveFunction();
}

TEST_CASE("state scope push/pop", "[state]") {
  state.reset();
  state.enterFunction("test", "function", 0);

  std::string reg = state.allocRegisters("u32").reg;
  state.declareVar("outer", "u32", reg);

  state.pushScope();
  REQUIRE(state.varExists("outer")); // Inherited from parent
  state.declareVar("inner", "u32", state.allocRegisters("u32").reg);
  REQUIRE(state.varExists("inner"));
  state.popScope();

  REQUIRE(state.varExists("outer"));
  REQUIRE(!state.varExists("inner")); // Gone after pop

  state.leaveFunction();
}

TEST_CASE("state label generation", "[state]") {
  state.reset();
  state.enterFunction("myFunc", "function", 0);

  std::string label1 = state.generateLabel();
  std::string label2 = state.generateLabel();
  REQUIRE(label1 != label2);
  REQUIRE(label1.find("LABEL_myFunc_") == 0);

  state.leaveFunction();
}

TEST_CASE("state const and modify tracking", "[state]") {
  state.reset();
  state.enterFunction("test", "function", 0);

  std::string reg = state.allocRegisters("u32").reg;
  state.declareVar("x", "u32", reg, true); // const

  state.markVarModified("x");
  const VarDef *v = state.getRequiredVar("x", "test", "");
  REQUIRE(v->modifyCount == 1);
  REQUIRE(v->isConst == true);

  state.leaveFunction();
}

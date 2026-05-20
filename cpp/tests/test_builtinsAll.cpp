#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "pipeline.h"

#include <string>

#define TRANSPILE(src)                                                         \
  rspl::transpileSource(src, {.rspqWrapper = false})

#define REQUIRE_NO_WARN(r) REQUIRE(r.warn.empty())
#define REQUIRE_ASM(r, expected) REQUIRE(r.asm_ == expected)
#define REQUIRE_THROWS_MSG(src, msg)                                           \
  REQUIRE_THROWS_WITH(rspl::transpileSource(src, {.rspqWrapper = false}),      \
                      Catch::Matchers::ContainsSubstring(msg))

// ==========================================================================
// MFC0 Reads — parameterized via macro
// ==========================================================================

#define MFC0_READ_TESTS(name, cop0Reg)                                         \
  TEST_CASE("Builtins - " name "() - basic", "[mfc0_reads]") {                 \
    auto r = TRANSPILE("function test() {\n        u32<$t0> a = " name         \
                       "();\n      }");                                        \
    REQUIRE_NO_WARN(r);                                                        \
    REQUIRE_ASM(r, "test:\n  mfc0 $t0, " cop0Reg "\n  jr $ra\n  nop");        \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with no left side",                 \
            "[mfc0_reads]") {                                                  \
    REQUIRE_THROWS_MSG("function test() {\n        " name                      \
                       "();\n      }",                                         \
                       "must have a left side");                               \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with arguments", "[mfc0_reads]") {  \
    REQUIRE_THROWS_MSG(                                                        \
        "function test() {\n        u32<$t0> a = " name                        \
        "(42);\n      }",                                                      \
        "requires no arguments");                                              \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with vector left side",             \
            "[mfc0_reads]") {                                                  \
    REQUIRE_THROWS_MSG(                                                        \
        "function test() {\n        vec16<$v01> a = " name                     \
        "();\n      }",                                                        \
        "scalar variable");                                                    \
  }

MFC0_READ_TESTS("get_dma_busy", "COP0_DMA_BUSY")
MFC0_READ_TESTS("get_rdp_start", "COP0_DP_START")
MFC0_READ_TESTS("get_rdp_end", "COP0_DP_END")
MFC0_READ_TESTS("get_rdp_current", "COP0_DP_CURRENT")

// ==========================================================================
// MTC0 Writes — parameterized via macro
// ==========================================================================

#define MTC0_WRITE_TESTS(name, cop0Reg)                                        \
  TEST_CASE("Builtins - " name "() - basic - scalar variable",                 \
            "[mtc0_writes]") {                                                 \
    auto r = TRANSPILE("function test() {\n        u32<$t0> a;\n        " name \
                       "(a);\n      }");                                       \
    REQUIRE_NO_WARN(r);                                                        \
    REQUIRE_ASM(r, "test:\n  mtc0 $t0, " cop0Reg "\n  jr $ra\n  nop");        \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - basic - literal", "[mtc0_writes]") {      \
    auto r = TRANSPILE("function test() {\n        " name "(42);\n      }");   \
    REQUIRE_NO_WARN(r);                                                        \
    REQUIRE_ASM(r, "test:\n  addiu $at, $zero, 42\n  mtc0 $at, " cop0Reg      \
                   "\n  jr $ra\n  nop");                                       \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with left side",                    \
            "[mtc0_writes]") {                                                 \
    REQUIRE_THROWS_MSG(                                                        \
        "function test() {\n        u32<$t0> a = " name                        \
        "(42);\n      }",                                                      \
        "must not have a left side");                                          \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with no argument",                  \
            "[mtc0_writes]") {                                                 \
    REQUIRE_THROWS_MSG("function test() {\n        " name "();\n      }",      \
                       "requires 1 scalar");                                   \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with vector argument",              \
            "[mtc0_writes]") {                                                 \
    REQUIRE_THROWS_MSG(                                                        \
        "function test() {\n        vec16<$v01> a;\n        " name             \
        "(a);\n      }",                                                       \
        "scalar argument");                                                    \
  }

MTC0_WRITE_TESTS("set_rdp_start", "COP0_DP_START")
MTC0_WRITE_TESTS("set_rdp_end", "COP0_DP_END")
MTC0_WRITE_TESTS("set_rdp_current", "COP0_DP_CURRENT")
MTC0_WRITE_TESTS("set_dma_addr_rsp", "COP0_DMA_SPADDR")
MTC0_WRITE_TESTS("set_dma_addr_rdram", "COP0_DMA_RAMADDR")
MTC0_WRITE_TESTS("set_dma_write", "COP0_DMA_WRITE")
MTC0_WRITE_TESTS("set_dma_read", "COP0_DMA_READ")

// ==========================================================================
// clear_vcc
// ==========================================================================

TEST_CASE("Builtins - clear_vcc() - basic", "[vcc]") {
  auto r = TRANSPILE(R"(function test() {
      clear_vcc();
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  vsubc $v29, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Builtins - clear_vcc() - fails with left side", "[vcc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = clear_vcc();
    })",
                     "must not have a left side");
}

TEST_CASE("Builtins - clear_vcc() - fails with arguments", "[vcc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      clear_vcc(42);
    })",
                     "requires no arguments");
}

// ==========================================================================
// get_acc
// ==========================================================================

TEST_CASE("Builtins - get_acc() - basic", "[get_acc]") {
  auto r = TRANSPILE(R"(function test() {
      vec32<$v01> a = get_acc();
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  vsar $v01, COP2_ACC_HI
  vsar $v02, COP2_ACC_MD
  jr $ra
  nop)");
}

TEST_CASE("Builtins - get_acc() - fails with no left side", "[get_acc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      get_acc();
    })",
                     "must have a left side");
}

TEST_CASE("Builtins - get_acc() - fails with arguments", "[get_acc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec32<$v01> a = get_acc(42);
    })",
                     "requires no arguments");
}

TEST_CASE("Builtins - get_acc() - fails with scalar left side",
          "[get_acc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = get_acc();
    })",
                     "vector variable");
}

TEST_CASE("Builtins - get_acc() - fails with vec16 left side", "[get_acc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec16<$v01> a = get_acc();
    })",
                     "vec32");
}

// ==========================================================================
// get_acc_high / get_acc_mid / get_acc_low
// ==========================================================================

#define ACC_SINGLE_TESTS(name, cop2Reg)                                        \
  TEST_CASE("Builtins - " name "() - basic", "[acc_single]") {                 \
    auto r = TRANSPILE("function test() {\n        vec16<$v03> a = " name      \
                       "();\n      }");                                        \
    REQUIRE_NO_WARN(r);                                                        \
    REQUIRE_ASM(r, "test:\n  vsar $v03, " cop2Reg "\n  jr $ra\n  nop");       \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with scalar left side",             \
            "[acc_single]") {                                                  \
    REQUIRE_THROWS_MSG(                                                        \
        "function test() {\n        u32<$t0> a = " name "();\n      }",        \
        "vector variable");                                                    \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with vec32 left side",              \
            "[acc_single]") {                                                  \
    REQUIRE_THROWS_MSG(                                                        \
        "function test() {\n        vec32<$v01> a = " name                     \
        "();\n      }",                                                        \
        "vec16");                                                              \
  }                                                                            \
  TEST_CASE("Builtins - " name "() - fails with arguments",                    \
            "[acc_single]") {                                                  \
    REQUIRE_THROWS_MSG(                                                        \
        "function test() {\n        vec16<$v01> a = " name                     \
        "(1);\n      }",                                                       \
        "requires no arguments");                                              \
  }

ACC_SINGLE_TESTS("get_acc_high", "COP2_ACC_HI")
ACC_SINGLE_TESTS("get_acc_mid", "COP2_ACC_MD")
ACC_SINGLE_TESTS("get_acc_low", "COP2_ACC_LO")

// ==========================================================================
// get_vcc
// ==========================================================================

TEST_CASE("Builtins - get_vcc() - basic", "[vcc]") {
  auto r = TRANSPILE(R"(function test() {
      u32<$t0> a = get_vcc();
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  cfc2 $t0, $vcc
  jr $ra
  nop)");
}

TEST_CASE("Builtins - get_vcc() - fails with no left side", "[vcc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      get_vcc();
    })",
                     "must have a left side");
}

TEST_CASE("Builtins - get_vcc() - fails with arguments", "[vcc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = get_vcc(1);
    })",
                     "requires no arguments");
}

TEST_CASE("Builtins - get_vcc() - fails with vector left side", "[vcc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec16<$v01> a = get_vcc();
    })",
                     "scalar variable");
}

// ==========================================================================
// set_vcc
// ==========================================================================

TEST_CASE("Builtins - set_vcc() - basic - scalar", "[vcc]") {
  auto r = TRANSPILE(R"(function test() {
      u32<$t0> a;
      set_vcc(a);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  ctc2 $t0, $vcc
  jr $ra
  nop)");
}

TEST_CASE("Builtins - set_vcc() - basic - literal", "[vcc]") {
  auto r = TRANSPILE(R"(function test() {
      set_vcc(1);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  addiu $at, $zero, 1
  ctc2 $at, $vcc
  jr $ra
  nop)");
}

TEST_CASE("Builtins - set_vcc() - fails with left side", "[vcc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = set_vcc(1);
    })",
                     "must not have a left side");
}

TEST_CASE("Builtins - set_vcc() - fails with no argument", "[vcc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      set_vcc();
    })",
                     "requires 1 scalar");
}

TEST_CASE("Builtins - set_vcc() - fails with vector argument", "[vcc]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec16<$v01> a;
      set_vcc(a);
    })",
                     "scalar argument");
}

// ==========================================================================
// Inline ASM — asm() and asm_op()
// ==========================================================================

TEST_CASE("Builtins - asm() - basic - string only", "[asm]") {
  auto r = TRANSPILE(R"(function test() {
      asm("nop");
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  nop # inline-ASM
  jr $ra
  nop)");
}

TEST_CASE("Builtins - asm() - with substitution - number", "[asm]") {
  auto r = TRANSPILE(R"(function test() {
      asm("addiu $t0, $zero, %0", 42);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE(r.asm_.find("addiu $t0, $zero, 42") != std::string::npos);
  REQUIRE(r.asm_.find("# inline-ASM") != std::string::npos);
}

TEST_CASE("Builtins - asm() - with substitution - register", "[asm]") {
  auto r = TRANSPILE(R"(function test() {
      u32<$t1> a;
      asm("addu $t0, $zero, %0", a);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE(r.asm_.find("addu $t0, $zero, $t1") != std::string::npos);
  REQUIRE(r.asm_.find("# inline-ASM") != std::string::npos);
}

TEST_CASE("Builtins - asm() - fails with left side", "[asm]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = asm("nop");
    })",
                     "cannot have a left side");
}

TEST_CASE("Builtins - asm() - fails with first arg not string", "[asm]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a;
      asm(42);
    })",
                     "first argument to be a string");
}

TEST_CASE("Builtins - asm_op() - basic", "[asm_op]") {
  auto r = TRANSPILE(R"(function test() {
      u32<$t0> a;
      asm_op("mtc0", a);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  mtc0 $t0
  jr $ra
  nop)");
}

TEST_CASE("Builtins - asm_op() - fails with left side", "[asm_op]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = asm_op("nop");
    })",
                     "cannot have a left side");
}

TEST_CASE("Builtins - asm_op() - fails with first arg not string",
          "[asm_op]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      asm_op(42);
    })",
                     "opcode");
}

// ==========================================================================
// dma_await
// ==========================================================================

TEST_CASE("Builtins - dma_await() - basic", "[dma]") {
  auto r = TRANSPILE(R"(function test() {
      dma_await();
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  jal DMAWaitIdle
  nop
  jr $ra
  nop)");
}

TEST_CASE("Builtins - dma_await() - fails with left side", "[dma]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = dma_await();
    })",
                     "cannot have a left side");
}

TEST_CASE("Builtins - dma_await() - fails with arguments", "[dma]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      dma_await(1);
    })",
                     "requires no arguments");
}

// ==========================================================================
// get_cmd_address
// ==========================================================================

TEST_CASE("Builtins - get_cmd_address() - basic with offset", "[cmd]") {
  auto r = TRANSPILE(R"(function test() {
      u32<$t1> a = get_cmd_address(12);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  addiu $t1, $gp, %lo(RSPQ_DMEM_BUFFER) + 12
  jr $ra
  nop)");
}

TEST_CASE("Builtins - get_cmd_address() - fails with no left side",
          "[cmd]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      get_cmd_address();
    })",
                     "must have a left side");
}

TEST_CASE("Builtins - get_cmd_address() - fails with too many arguments",
          "[cmd]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = get_cmd_address(1, 2);
    })",
                     "zero or one argument");
}

TEST_CASE("Builtins - get_cmd_address() - fails with non-number argument",
          "[cmd]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a;
      u32<$t1> b = get_cmd_address(a);
    })",
                     "number");
}

TEST_CASE("Builtins - get_cmd_address() - fails with vector left side",
          "[cmd]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec16<$v01> a = get_cmd_address();
    })",
                     "scalar variable");
}

// ==========================================================================
// load_arg
// ==========================================================================

TEST_CASE("Builtins - load_arg() - basic with offset", "[cmd]") {
  auto r = TRANSPILE(R"(function test() {
      u32<$t1> a = load_arg(8);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  lw $t1, %lo(RSPQ_DMEM_BUFFER + 8)($gp)
  jr $ra
  nop)");
}

TEST_CASE("Builtins - load_arg() - fails with no left side", "[cmd]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      load_arg();
    })",
                     "must have a left side");
}

TEST_CASE("Builtins - load_arg() - fails with too many arguments", "[cmd]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = load_arg(1, 2);
    })",
                     "zero or one argument");
}

TEST_CASE("Builtins - load_arg() - fails with non-number argument", "[cmd]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a;
      u32<$t1> b = load_arg(a);
    })",
                     "number");
}

TEST_CASE("Builtins - load_arg() - fails with vector left side", "[cmd]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec16<$v01> a = load_arg();
    })",
                     "scalar variable");
}

// ==========================================================================
// max / min
// ==========================================================================

TEST_CASE("Builtins - max() - basic", "[maxmin]") {
  auto r = TRANSPILE(R"(function test() {
      vec16<$v01> a, b;
      vec16<$v03> c = max(a, b);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE(r.asm_.find("vge") != std::string::npos);
}

TEST_CASE("Builtins - max() - fails with wrong arg count", "[maxmin]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec16<$v01> a;
      vec16<$v03> c = max(a);
    })",
                     "exactly two arguments");
}

TEST_CASE("Builtins - max() - fails with type mismatch", "[maxmin]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec16<$v01> a;
      vec32<$v03> b;
      vec16<$v05> c = max(a, b);
    })",
                     "can only use vec16");
}

TEST_CASE("Builtins - min() - basic", "[maxmin]") {
  auto r = TRANSPILE(R"(function test() {
      vec16<$v01> a, b;
      vec16<$v03> c = min(a, b);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE(r.asm_.find("vlt") != std::string::npos);
}

TEST_CASE("Builtins - min() - fails with wrong arg count", "[maxmin]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      vec16<$v01> a;
      vec16<$v03> c = min(a);
    })",
                     "exactly two arguments");
}

// ==========================================================================
// assert
// ==========================================================================

TEST_CASE("Builtins - assert() - basic", "[assert]") {
  auto r = TRANSPILE(R"(function test() {
      assert(42);
    })");
  REQUIRE_NO_WARN(r);
  REQUIRE_ASM(r, R"(test:
  lui $at, 42
  j assertion_failed
  nop
  jr $ra
  nop)");
}

TEST_CASE("Builtins - assert() - fails with left side", "[assert]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a = assert(1);
    })",
                     "cannot have a left side");
}

TEST_CASE("Builtins - assert() - fails with wrong arg count", "[assert]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      assert(1, 2);
    })",
                     "exactly one argument");
}

TEST_CASE("Builtins - assert() - fails with non-number argument", "[assert]") {
  REQUIRE_THROWS_MSG(R"(function test() {
      u32<$t0> a;
      assert(a);
    })",
                     "number");
}
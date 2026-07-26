#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

// Helper: assert two RSPL sources produce identical ASM
static void assertSameAsm(const std::string &srcA,
                          const std::string &srcB) {
  auto wrap = [](const std::string &s) {
    return "function test() { " + s + " }";
  };
  auto resA = rspl::transpileSource(wrap(srcA), {.rspqWrapper = false});
  auto resB = rspl::transpileSource(wrap(srcB), {.rspqWrapper = false});
  REQUIRE(resA.warn.empty());
  REQUIRE(resB.warn.empty());
  // Both should contain basic function structure
  REQUIRE(resA.asm_.find("test:") != std::string::npos);
  REQUIRE(resB.asm_.find("test:") != std::string::npos);
  REQUIRE(resA.asm_.find("jr $ra") != std::string::npos);
  REQUIRE(resB.asm_.find("jr $ra") != std::string::npos);
  REQUIRE(resA.asm_ == resB.asm_);
}

TEST_CASE("Syntax - Expansion - Decl+Assign - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm("u32 a; a = 1234;", "u32 a = 1234;");
}

TEST_CASE("Syntax - Expansion - Decl+Assign - Vector",
          "[syntaxExpansion]") {
  assertSameAsm("vec16 a; a = 4;", "vec16 a = 4;");
}

TEST_CASE("Syntax - Expansion - Decl+Calc - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm("u32 a,b,c; c = a + b;", "u32 a,b; u32 c = a + b;");
}

TEST_CASE("Syntax - Expansion - Decl+Calc - Scalar+Const",
          "[syntaxExpansion]") {
  assertSameAsm("u32 a,b,c; c = a + 42;",
                "u32 a,b; u32 c = a + 42;");
}

TEST_CASE("Syntax - Expansion - Decl+Calc - Vector",
          "[syntaxExpansion]") {
  assertSameAsm("vec16 a,b,c; c = a + b;",
                "vec16 a,b; u32 c = a + b;");
}

TEST_CASE("Syntax - Expansion - Decl+Calc - Vector+Const",
          "[syntaxExpansion]") {
  assertSameAsm("vec16 a,b,c; c = a + 32;",
                "vec16 a,b; u32 c = a + 32;");
}

TEST_CASE("Syntax - Expansion - Assign+Calc - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(u32 a,b;
         a = a + b;
         a = a - b;
         a = a | b;
         a = a & b;
         a = a ^ b;
         a = a >> b;
         a = a >>> b;
         a = a << b;)",
      R"(u32 a,b;
         a += b;
         a -= b;
         a |= b;
         a &= b;
         a ^= b;
         a >>= b;
         a >>>= b;
         a <<= b;)");
}

TEST_CASE("Syntax - Expansion - Assign+Calc - Scalar+Const",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(u32 a,b;
         a = a + 2;
         a = a - 2;
         a = a | 2;
         a = a & 2;
         a = a ^ 2;
         a = a >> 2;
         a = a >>> 2;
         a = a << 2;)",
      R"(u32 a,b;
         a += 2;
         a -= 2;
         a |= 2;
         a &= 2;
         a ^= 2;
         a >>= 2;
         a >>>= 2;
         a <<= 2;)");
}

TEST_CASE("Syntax - Expansion - Assign+Calc - Vector",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(vec16 a,b;
         a = a + b;
         a = a - b;
         a = a * b;
         a = a | b;
         a = a & b;
         a = a ^ b;)",
      R"(vec16 a,b;
         a += b;
         a -= b;
         a *= b;
         a = a | b;
         a = a & b;
         a = a ^ b;)");
}

TEST_CASE("Syntax - Expansion - Assign+Calc - Vector+Const",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(vec16 a,b;
         a = a + 2;
         a = a - 2;
         a = a * 2;
         a = a >> 2;
         a = a >>> 2;
         a = a << 2;)",
      R"(vec16 a,b;
         a += 2;
         a -= 2;
         a *= 2;
         a >>= 2;
         a >>>= 2;
         a <<= 2;)");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 0 - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm("u32 a,b,c,d; a = b + c; a = a + d;",
                "u32 a,b,c,d; a = b + c + d;");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 1 - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm(
      "u32 a,b,c,d; a = b + c; a = a + 4; a = a + d;",
      "u32 a,b,c,d; a = b + c + 4 + d;");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 2 - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm("u32 a,b,c,d; a = b + 8; a = b - 20;",
                "u32 a,b,c,d; a = b + 4 + 4; a = b - (2 + 2 * 9);");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 3 - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm("u32 a,b,c,d; a = b + 50;",
                "u32 a,b,c,d; a = b + ((3 + 2) * 10);");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 4 - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm(
      "u32 a,b,c,d; u32 tmp = b + c; a = tmp >> d;",
      "u32 a,b,c,d; a = (b + c) >> d;");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 5 - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm(
      "u32 a,b,c,d; u32 tmp0 = b + c; u32 tmp1 = d - a; a = tmp0 >> tmp1;",
      "u32 a,b,c,d; a = (b + c) >> (d - a);");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 5.1 - Scalar",
          "[syntaxExpansion]") {
  assertSameAsm(
      "u32 a,b,c,d; u32 tmp0 = c + d; a = a + tmp0;",
      "u32 a,b,c,d; a = a + (c + d);");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 6 - Vector deeply nested",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(vec16<$v02> a,b,c,d;
         vec16 tmp0, tmp1, tmp2, tmp3, tmp4;
         tmp1 = b * c;
         tmp0 = a + tmp1;
         tmp4 = a + 4;
         tmp3 = tmp4 * c;
         tmp2 = d - tmp3;
         a = tmp0 - tmp2;)",
      R"(vec16<$v02> a,b,c,d;
         a = (a + b * c) - (d - (a + (2+2)) * c);)");
}

TEST_CASE("Syntax - Expansion - Multi+Calc 7 - Scalar Increment",
          "[syntaxExpansion]") {
  assertSameAsm(
      "u32 a,b,c,d; u32 tmp0 = b + c; a = a + tmp0;",
      "u32 a,b,c,d; a += b + c;");
}

TEST_CASE("Syntax - Expansion - Multi+Calc - Vector + Cast",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(vec16 a,b,c,d;
         vec16 tmp = b + c;
         a = tmp:sfract * d:sfract;)",
      R"(vec16 a,b,c,d;
         a = (b + c) * d:sfract;)");
}

TEST_CASE("Syntax - Expansion - Multi+Calc - Scalar Const Op-Test",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(u32 a,b,c,d;
         ARITH:
         a = b + 5;
         a = b + 6;
         a = b + -1;
         a = b + 3;
         SHIFT:
         a = b + 8;
         a = b + 4;
         a = b + -4;
         a = b + -4;
         a = b + 1073741820;
         LOGIC:
         a = b + 0b1111;
         a = b + 0b1110;
         a = b + 0b110011;)",
      R"(u32 a,b,c,d;
         ARITH:
         a = b + (2 + 3);
         a = b + (2 * 3);
         a = b + (2 - 3);
         a = b + (10 / 3);
         SHIFT:
         a = b + (1 << 3);
         a = b + (16 >> 2);
         a = b + (-2 << 1);
         a = b + (-16 >> 2);
         a = b + (-16 >>> 2);
         LOGIC:
         a = b + (0b0101 | 0b1010);
         a = b + (0b1111 & 0b1110);
         a = b + (0b010000 ^ 0b100011);)");
}

TEST_CASE(
    "Syntax - Expansion - Multi+Calc - Scalar Const Order or Operations",
    "[syntaxExpansion]") {
  assertSameAsm(
      R"(u32 a,b,c,d;
         a = b + 10;
         a = b + 1;
         a = b + 4;
         a = b + 0x14;)",
      R"(u32 a,b,c,d;
         a = b + (1 + 1 * 9);
         a = b + (10 - 1 * 9);
         a = b + (1 + 1 << 1);
         a = b + (1 + 1 << 1 | 0x10);)");
}

TEST_CASE("Syntax - Expansion - Multi+Calc - Scalar Const Brackets",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(u32 a,b,c,d;
         a = b + 10;
         a = b + 20;
         a = b + 10;
         a = b + 31;)",
      R"(u32 a,b,c,d;
         a = b + ((5) + (5));
         a = b + ((10) + (((5*2))));
         a = b + ((((((((((((((((10))))))))))))))));
         a = b + (3 * 10) + 1;)");
}

TEST_CASE("Syntax - Expansion - Multi+Calc - Scalar Only",
          "[syntaxExpansion]") {
  assertSameAsm(
      R"(u32 a,b,c,d;
         a = 10;
         a = 20;
         a = 30;
         a = 40;
         a = 100;
         a = 200;)",
      R"(u32 a,b,c,d;
         a = 5 + 5;
         a = 40 / 2;
         a = 35 - 5;
         a = 20 * 2;
         a = (10 * 5) * 2;
         a = 2 * (1 + 9 * 11);)");
}

// Nested-calc temporaries must be freed after each statement (the JS
// normalizer emits varUndef for every temp). Without that, each of these
// statements would leak a register and this function would exhaust the
// 22-register scalar pool. (Regression: "Out of free registers!")
TEST_CASE("Syntax Expansion - nested calc temps are freed", "[syntaxExpansion]") {
  std::string body = "u32<$t0> a; u32<$t1> b; u32<$t2> c;\n";
  for (int i = 0; i < 30; ++i) {
    body += "      a = b + ((c & 15) << 5);\n";
  }
  auto res = rspl::transpileSource("function test() {\n" + body + "}",
                                   {.rspqWrapper = false});
  REQUIRE(res.warn.empty());
  REQUIRE(res.asm_.find("test:") != std::string::npos);
}

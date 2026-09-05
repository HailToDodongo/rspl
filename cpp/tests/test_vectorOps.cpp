#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <string>

TEST_CASE("Vector - Ops - Assign (vec32 vs vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res = a;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v01, $v00, $v03
  vor $v02, $v00, $v04
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (vec16 vs vec32:cast)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res;
      vec32<$v03> a;
      res = a:uint;
      res = a:sint;
      res:ufract = a:ufract;
      res:sfract = a:sfract;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v01, $v00, $v03
  vor $v01, $v00, $v03
  vor $v01, $v00, $v04
  vor $v01, $v00, $v04
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (vec16 vs vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      res = a;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v01, $v00, $v02
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (vec16 broadcast)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      res = a.yyyyYYYY;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v01, $v00, $v02.h1
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (vec32 broadcast)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res = a.yyyyYYYY;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v01, $v00, $v03.h1
  vor $v02, $v00, $v04.h1
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (swizzle, 2^x)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a.x = 2;
      b.x = 8;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmov $v01.e0, $v30.e6
  vmov $v02.e0, $v30.e4
  vmov $v03.e0, $v00.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (swizzle, float)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a.x = 10.25;
      b.x = 42.125;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  addiu $at, $zero, 10
  mtc2 $at, $v01.e0
  addiu $at, $zero, 42
  mtc2 $at, $v02.e0
  addiu $at, $zero, 8192
  mtc2 $at, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (swizzle, int-variable)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32 s;
      vec16<$v01> a;
      vec32<$v02> b;
      a.y = s;
      b.z = s;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  mtc2 $t0, $v01.e1
  mtc2 $t0, $v03.e2
  srl $at, $t0, 16
  mtc2 $at, $v02.e2
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (no-swizzle, int-variable)",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      u32 s;
      vec16<$v01> a;
      vec32<$v02> b;
      a = s;
      b = s;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  mtc2 $t0, $v01.e0
  vor $v01, $v00, $v01.e0
  mtc2 $t0, $v03.e0
  srl $at, $t0, 16
  mtc2 $at, $v02.e0
  vor $v02, $v00, $v02.e0
  vor $v03, $v00, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (swizzle, 0)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a.x = 0;
      b.x = 0;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmov $v01.e0, $v00.e0
  vmov $v02.e0, $v00.e0
  vmov $v03.e0, $v00.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (cast, swizzle, 0)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a;
      vec32<$v02> b;
      a:sint.x = 0;
      b:sfract.x = 0;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmov $v01.e0, $v00.e0
  vmov $v03.e0, $v00.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Assign (0)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a = 0;
      vec32<$v02> b = 0;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vxor $v01, $v00, $v00.e0
  vxor $v02, $v00, $v00.e0
  vxor $v03, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add (vec32 vs vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res += a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vaddc $v02, $v02, $v04.e0
  vadd $v01, $v01, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add (vec16 vs vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      res += a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vaddc $v01, $v01, $v02.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add (vec16 cast)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      res:uint += a.x;
      res:sint += a.x;
      res:sfract += a.x;
      res:ufract += a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // an operand without a cast follows the destination's view
  REQUIRE(result.asm_ == R"(test:
  vaddc $v01, $v01, $v02.e0
  vadd $v01, $v01, $v02.e0
  vadd $v01, $v01, $v02.e0
  vaddc $v01, $v01, $v02.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Sub (vec32 vs vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res -= a.y;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vsubc $v02, $v02, $v04.e1
  vsub $v01, $v01, $v03.e1
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Sub (vec16 vs vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      res -= a;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vsubc $v01, $v01, $v02.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec32 vs vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res *= a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudl $v29, $v02, $v04.e0
  vmadm $v29, $v01, $v04.e0
  vmadn $v02, $v02, $v03.e0
  vmadh $v01, $v01, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16 vs vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      res *= a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudn $v01, $v01, $v02.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16 cast)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      res:uint *= a.x;
      res:sint *= a.x;
      res:ufract *= a.x;
      res:sfract *= a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudn $v01, $v01, $v02.e0
  vmudh $v01, $v01, $v02.e0
  vmulu $v01, $v01, $v02.e0
  vmulf $v01, $v01, $v02.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - AND (vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v02> res16, a16;
      vec32<$v04> a32;

      res16 = a16 & a16;
      res16 = a32 & a16;
      res16 = a16 & a32;
      res16 = a32 & a32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vand $v02, $v03, $v03.v
  vand $v02, $v04, $v03.v
  vand $v02, $v03, $v04.v
  vand $v02, $v04, $v04.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - AND (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> res32, a32;
      vec16<$v06> a16;

      res32 = a16 & a16; A:
      res32 = a32 & a16; B:
      res32 = a16 & a32; C:
      res32 = a32 & a32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vand $v02, $v06, $v06.v
  vand $v03, $v00, $v00.v
  A:
  vand $v02, $v04, $v06.v
  vand $v03, $v05, $v00.v
  B:
  vand $v02, $v06, $v04.v
  vand $v03, $v00, $v05.v
  C:
  vand $v02, $v04, $v04.v
  vand $v03, $v05, $v05.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - OR (vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v02> res16, a16;
      vec32<$v04> a32;

      res16 = a16 | a16;
      res16 = a32 | a16;
      res16 = a16 | a32;
      res16 = a32 | a32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v02, $v03, $v03.v
  vor $v02, $v04, $v03.v
  vor $v02, $v03, $v04.v
  vor $v02, $v04, $v04.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - OR (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> res32, a32;
      vec16<$v06> a16;

      res32 = a16 | a16; AA:
      res32 = a32 | a16; BB:
      res32 = a16 | a32; CC:
      res32 = a32 | a32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vor $v02, $v06, $v06.v
  vor $v03, $v00, $v00.v
  AA:
  vor $v02, $v04, $v06.v
  vor $v03, $v05, $v00.v
  BB:
  vor $v02, $v06, $v04.v
  vor $v03, $v00, $v05.v
  CC:
  vor $v02, $v04, $v04.v
  vor $v03, $v05, $v05.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - XOR (vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v02> res16, a16;
      vec32<$v04> a32;

      res16 = a16 ^ a16;
      res16 = a32 ^ a16;
      res16 = a16 ^ a32;
      res16 = a32 ^ a32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vxor $v02, $v03, $v03.v
  vxor $v02, $v04, $v03.v
  vxor $v02, $v03, $v04.v
  vxor $v02, $v04, $v04.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - XOR (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> res32, a32;
      vec16<$v06> a16;

      res32 = a16 ^ a16; A:
      res32 = a32 ^ a16; B:
      res32 = a16 ^ a32; C:
      res32 = a32 ^ a32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vxor $v02, $v06, $v06.v
  vxor $v03, $v00, $v00.v
  A:
  vxor $v02, $v04, $v06.v
  vxor $v03, $v05, $v00.v
  B:
  vxor $v02, $v06, $v04.v
  vxor $v03, $v00, $v05.v
  C:
  vxor $v02, $v04, $v04.v
  vxor $v03, $v05, $v05.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - NOT (vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v02> res16, a16;
      vec32<$v04> a32;

      res16 = ~a16;
      res16 = ~a32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vnor $v02, $v03, $v00.v
  vnor $v02, $v04, $v00.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - NOT (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> res32, a32;
      vec16<$v06> a16;

      res32 = ~a16;
      res32 = ~a32;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vnor $v02, $v06, $v00.v
  vnor $v03, $v00, $v00.v
  vnor $v02, $v04, $v00.v
  vnor $v03, $v05, $v00.v
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Invert-Half (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      a.x = invert_half(a).x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vrcph $v03.e0, $v03.e0
  vrcpl $v04.e0, $v04.e0
  vrcph $v03.e0, $v00.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Invert-Half - all (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      a = invert_half(a);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vrcph $v03.e0, $v03.e0
  vrcpl $v04.e0, $v04.e0
  vrcph $v03.e0, $v00.e0
  vrcph $v03.e1, $v03.e1
  vrcpl $v04.e1, $v04.e1
  vrcph $v03.e1, $v00.e1
  vrcph $v03.e2, $v03.e2
  vrcpl $v04.e2, $v04.e2
  vrcph $v03.e2, $v00.e2
  vrcph $v03.e3, $v03.e3
  vrcpl $v04.e3, $v04.e3
  vrcph $v03.e3, $v00.e3
  vrcph $v03.e4, $v03.e4
  vrcpl $v04.e4, $v04.e4
  vrcph $v03.e4, $v00.e4
  vrcph $v03.e5, $v03.e5
  vrcpl $v04.e5, $v04.e5
  vrcph $v03.e5, $v00.e5
  vrcph $v03.e6, $v03.e6
  vrcpl $v04.e6, $v04.e6
  vrcph $v03.e6, $v00.e6
  vrcph $v03.e7, $v03.e7
  vrcpl $v04.e7, $v04.e7
  vrcph $v03.e7, $v00.e7
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Invert-SQRT-Half (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      a.x = invert_half_sqrt(a).x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vrsqh $v03.e0, $v03.e0
  vrsql $v04.e0, $v04.e0
  vrsqh $v03.e0, $v00.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Invert (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      a = invert(a);
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vrcph $v03.e0, $v03.e0
  vrcpl $v04.e0, $v04.e0
  vrcph $v03.e0, $v00.e0
  vrcph $v03.e1, $v03.e1
  vrcpl $v04.e1, $v04.e1
  vrcph $v03.e1, $v00.e1
  vrcph $v03.e2, $v03.e2
  vrcpl $v04.e2, $v04.e2
  vrcph $v03.e2, $v00.e2
  vrcph $v03.e3, $v03.e3
  vrcpl $v04.e3, $v04.e3
  vrcph $v03.e3, $v00.e3
  vrcph $v03.e4, $v03.e4
  vrcpl $v04.e4, $v04.e4
  vrcph $v03.e4, $v00.e4
  vrcph $v03.e5, $v03.e5
  vrcpl $v04.e5, $v04.e5
  vrcph $v03.e5, $v00.e5
  vrcph $v03.e6, $v03.e6
  vrcpl $v04.e6, $v04.e6
  vrcph $v03.e6, $v00.e6
  vrcph $v03.e7, $v03.e7
  vrcpl $v04.e7, $v04.e7
  vrcph $v03.e7, $v00.e7
  vmudn $v04, $v04, $v30.e6
  vmadh $v03, $v03, $v30.e6
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Shift Left (vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v02> a, b;
      b = a << 1;
      b = a << 4;
      b = a << 15;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudn $v03, $v02, $v30.e6
  vmudn $v03, $v02, $v30.e3
  vmudn $v03, $v02, $v31.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Shift Right Arithmetic (vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v02> a, b;
      b = a >> 1;
      b = a >> 4;
      b = a >> 15;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudm $v03, $v02, $v31.e0
  vmudm $v03, $v02, $v31.e3
  vmudm $v03, $v02, $v30.e6
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Shift Right Logical (vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v02> a, b;
      b = a >>> 1;
      b = a >>> 4;
      b = a >>> 15;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudl $v03, $v02, $v31.e0
  vmudl $v03, $v02, $v31.e3
  vmudl $v03, $v02, $v30.e6
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Shift Left (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> a, b;
      b = a << 1;
      b = a << 4;
      b = a << 15;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudl $v04, $v03, $v30.e6
  vmadn $v04, $v02, $v30.e6
  vmudn $v05, $v03, $v30.e6
  vmudl $v04, $v03, $v30.e3
  vmadn $v04, $v02, $v30.e3
  vmudn $v05, $v03, $v30.e3
  vmudl $v04, $v03, $v31.e0
  vmadn $v04, $v02, $v31.e0
  vmudn $v05, $v03, $v31.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Shift Left (vec32 self-assign)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> a, b;
      a = a << 1;
      a = a << 4;
      a = a << 15;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudl $v29, $v03, $v30.e6
  vmadn $v02, $v02, $v30.e6
  vmudn $v03, $v03, $v30.e6
  vmudl $v29, $v03, $v30.e3
  vmadn $v02, $v02, $v30.e3
  vmudn $v03, $v03, $v30.e3
  vmudl $v29, $v03, $v31.e0
  vmadn $v02, $v02, $v31.e0
  vmudn $v03, $v03, $v31.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Shift Left (vec16 = vec32 << X)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> b;
      vec16<$v04> a;
      a = b << 1;
      a = b << 4;
      a = b << 15;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudl $v04, $v03, $v30.e6
  vmadn $v04, $v02, $v30.e6
  vmudl $v04, $v03, $v30.e3
  vmadn $v04, $v02, $v30.e3
  vmudl $v04, $v03, $v31.e0
  vmadn $v04, $v02, $v31.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Shift right Arithmetic (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> a, b;
      b = a >> 1;
      b = a >> 4;
      b = a >> 15;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudl $v05, $v03, $v31.e0
  vmadm $v04, $v02, $v31.e0
  vmadn $v05, $v00, $v00
  vmudl $v05, $v03, $v31.e3
  vmadm $v04, $v02, $v31.e3
  vmadn $v05, $v00, $v00
  vmudl $v05, $v03, $v30.e6
  vmadm $v04, $v02, $v30.e6
  vmadn $v05, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Shift right Logical (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> a, b;
      b = a >>> 1;
      b = a >>> 4;
      b = a >>> 15;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudl $v05, $v03, $v31.e0
  vmadn $v04, $v02, $v31.e0
  vmadn $v05, $v00, $v00
  vmudl $v05, $v03, $v31.e3
  vmadn $v04, $v02, $v31.e3
  vmadn $v05, $v00, $v00
  vmudl $v05, $v03, $v30.e6
  vmadn $v04, $v02, $v30.e6
  vmadn $v05, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("VectorOps - Multiply-accumulate +* - vec32",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
        vec32<$v01> a;
        vec32<$v03> b;
        vec32<$v05> res;
        res = a +* b;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadl $v05, $v02, $v04.v
  vmadm $v05, $v01, $v04.v
  vmadn $v06, $v02, $v03.v
  vmadh $v05, $v01, $v03.v
  jr $ra
  nop)");
}

TEST_CASE("VectorOps - Multiply vec16 * vec32 -> vec32",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
        vec16<$v01> a;
        vec32<$v03> b;
        vec32<$v05> res;
        res = a * b;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // Reordered to vec32 * vec16: vmudn drops the low slice straight into the
  // result and the vmadh that follows only touches the upper slices, so the
  // read-back of the accumulator's low half is not needed.
  REQUIRE(result.asm_ == R"(test:
  vmudn $v06, $v04, $v01.v
  vmadh $v05, $v03, $v01.v
  jr $ra
  nop)");
}

TEST_CASE("VectorOps - Multiply vec16 * vec32 with a swizzled vec32",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
        vec16<$v01> a;
        vec32<$v03> b;
        vec32<$v05> res;
        res = a * b.wwwwWWWW;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // The lane select can only sit on `vt`, so the vec32 has to stay on the
  // right and the low half must be read back from the accumulator.
  REQUIRE(result.asm_ == R"(test:
  vmudm $v06, $v01, $v04.h3
  vmadh $v05, $v01, $v03.h3
  vmadn $v06, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("VectorOps - Multiply rejects a swizzle on the left operand",
          "[vectorOps]") {
  const char *src = R"(function test() {
        vec16<$v01> a;
        vec32<$v03> b;
        vec32<$v05> res;
        res = b.wwwwWWWW * a;
      })";
  REQUIRE_THROWS_AS(rspl::transpileSource(src, {.rspqWrapper = false}),
                    std::runtime_error);
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    INFO(e.what());
    REQUIRE(std::string(e.what()).find("swizzle on the right side") !=
            std::string::npos);
  }
}

TEST_CASE("VectorOps - Half-move vec32 xyzw=XYZW (upper to lower)",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
        vec32<$v01> res, a;
        res.xyzw = a.XYZW;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)
  sdv $v03, 8, 0, $at
  sdv $v04, 8, 8, $at
  ldv $v01, 0, 0, $at
  ldv $v02, 0, 8, $at
  jr $ra
  nop)");
}

TEST_CASE("VectorOps - Half-move vec32 XYZW=xyzw (lower to upper)",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
        vec32<$v01> res, a;
        res.XYZW = a.xyzw;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)
  sdv $v03, 0, 0, $at
  sdv $v04, 0, 8, $at
  ldv $v01, 8, 0, $at
  ldv $v02, 8, 8, $at
  jr $ra
  nop)");
}

TEST_CASE("VectorOps - Half-move vec16 xyzw=XYZW (upper to lower)",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
        vec16<$v01> res, a;
        res.xyzw = a.XYZW;
      })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  ori $at, $zero, %lo(RSPQ_SCRATCH_MEM)
  sdv $v02, 8, 0, $at
  ldv $v01, 0, 0, $at
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec32 vs vec32:ufract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res *= a:ufract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudl $v02, $v02, $v04.e0
  vmadm $v01, $v01, $v04.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16 vs vec32:ufract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      vec16<$v05> b;
      res = b * a:ufract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudm $v01, $v05, $v04.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16 vs vec16 -> vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res;
      vec16<$v03> a, b;
      res = a * b.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudh $v02, $v03, $v04.e0
  vsar $v01, COP2_ACC_HI
  vsar $v02, COP2_ACC_MD
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16 vs vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      vec16<$v05> b;
      res = b * a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudm $v02, $v05, $v04.e0
  vmadh $v01, $v05, $v03.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16 vs vec32 -> vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      vec32<$v03> b;
      res = a * b.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudm $v01, $v02, $v04.e0
  vmadh $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16:sint vs vec16:sint)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:sint * b:sint.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudh $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16:ufract vs vec16:ufract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:ufract * b:ufract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmulu $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16:sfract vs vec16:sfract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:sfract * b:sfract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmulf $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16:sint vs vec16:ufract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:sint * b:ufract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudm $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Mul (vec16:ufract vs vec16:sint)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:ufract * b:sint.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudn $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec32 vs vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res = res +* a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadl $v29, $v02, $v04.e0
  vmadm $v29, $v01, $v04.e0
  vmadn $v02, $v02, $v03.e0
  vmadh $v01, $v01, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec32 vs vec32:ufract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res = res +* a:ufract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadl $v02, $v02, $v04.e0
  vmadm $v01, $v01, $v04.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16 vs vec32:ufract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      vec16<$v05> b;
      res = b +* a:ufract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadm $v01, $v05, $v04.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec32 vs vec32, cast sfract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      res:sfract = res +* a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadl $v29, $v02, $v04.e0
  vmadm $v29, $v01, $v04.e0
  vmadn $v02, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16 vs vec16 -> vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res;
      vec16<$v03> a, b;
      res = a +* b.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadh $v02, $v03, $v04.e0
  vsar $v01, COP2_ACC_HI
  vsar $v02, COP2_ACC_MD
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16 vs vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v01> res, a;
      vec16<$v05> b;
      res = b +* a.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadm $v02, $v05, $v04.e0
  vmadh $v01, $v05, $v03.e0
  vmadn $v02, $v00, $v00
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16 vs vec32 -> vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a;
      vec32<$v03> b;
      res = a +* b.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadm $v01, $v02, $v04.e0
  vmadh $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16:sint vs vec16:sint)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:sint +* b:sint.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadh $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16:ufract vs vec16:ufract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:ufract +* b:ufract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmacu $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16:sfract vs vec16:sfract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:sfract +* b:sfract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmacf $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16:sint vs vec16:ufract)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:sint +* b:ufract.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadm $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add-Mul (vec16:ufract vs vec16:sint)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> res, a, b;
      res = a:ufract +* b:sint.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmadn $v01, $v02, $v03.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Logic with pow2 constant (vec16)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec16<$v01> a;
      a &= 0x400;
      a |= 16;
      a ^= 0x8000;
      a = a & 2;
      a &= 0;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vand $v01, $v01, $v31.e5
  vor $v01, $v01, $v30.e3
  vxor $v01, $v01, $v31.e0
  vand $v01, $v01, $v30.e6
  vand $v01, $v01, $v00.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Logic with pow2 constant (vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> b;
      b &= 0x400;
      b ^= 4;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // the constant's fraction bits are zero -> fract half uses the zero lane
  REQUIRE(result.asm_ == R"(test:
  vand $v02, $v02, $v31.e5
  vand $v03, $v03, $v00.e0
  vxor $v02, $v02, $v30.e5
  vxor $v03, $v03, $v00.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Logic with non-pow2 constant throws", "[vectorOps]") {
  const char *src = R"(function test() {
      vec16<$v01> a;
      a &= 3;
    })";
  REQUIRE_THROWS_AS(rspl::transpileSource(src, {.rspqWrapper = false}),
                    std::runtime_error);
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()).find("powers of two") != std::string::npos);
  }
}

static void requireVecThrowsWith(const char *src, const char *msgPart) {
  REQUIRE_THROWS_AS(rspl::transpileSource(src, {.rspqWrapper = false}),
                    std::runtime_error);
  try {
    rspl::transpileSource(src, {.rspqWrapper = false});
  } catch (const std::runtime_error &e) {
    INFO(e.what());
    REQUIRE(std::string(e.what()).find(msgPart) != std::string::npos);
  }
}

TEST_CASE("Vector - Ops - Add on vec32 fraction view with uncast operand",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> nrT;
      vec16<$v16> normShift;
      nrT:ufract += normShift.x;
      nrT:ufract += 2;
      nrT:sfract += normShift.x;
      nrT:ufract += normShift:ufract.x;
      nrT:sint += normShift.x;
      nrT:ufract -= normShift.x;
      nrT += normShift.x;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // the view decides how an uncast operand is read; a full vec32 target
  // keeps reading it as the integer half (with the carry into the int add)
  REQUIRE(result.asm_ == R"(test:
  vaddc $v03, $v03, $v16.e0
  vaddc $v03, $v03, $v30.e6
  vadd $v03, $v03, $v16.e0
  vaddc $v03, $v03, $v16.e0
  vadd $v02, $v02, $v16.e0
  vsubc $v03, $v03, $v16.e0
  vaddc $v03, $v03, $v00.e0
  vadd $v02, $v02, $v16.e0
  jr $ra
  nop)");
}

TEST_CASE("Vector - Ops - Add with an operand cast to the opposite view throws",
          "[vectorOps]") {
  requireVecThrowsWith(R"(function test() {
      vec32<$v02> nrT;
      vec16<$v16> a;
      nrT:ufract += a:sint.x;
    })", "integer operand on a fraction view");
  requireVecThrowsWith(R"(function test() {
      vec32<$v02> nrT;
      vec16<$v16> a;
      nrT:sint += a:ufract.x;
    })", "fraction operand on an integer view");
}

TEST_CASE("Vector - Ops - Mul (vec16 fraction * vec32)", "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test() {
      vec32<$v02> nrT;
      vec32<$v13> screenSize;
      vec16<$v01> nrD;
      vec16<$v05> r16;
      nrT = nrD:ufract * screenSize.wwwwWWWW;
      r16 = nrD:ufract * screenSize.wwwwWWWW;
      nrT = screenSize * nrD:ufract.xxxxXXXX;
    })",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // fraction on the left: it is `vs`, the vec32 halves are `vt` (the lane
  // swizzle can only sit on vt); no integer products, just a flush of the
  // high accumulator. The mirrored form keeps its existing pattern.
  REQUIRE(result.asm_ == R"(test:
  vmudl $v02, $v01, $v14.h3
  vmadn $v03, $v01, $v13.h3
  vmadh $v02, $v00, $v00
  vmudl $v05, $v01, $v14.h3
  vmadn $v05, $v01, $v13.h3
  vmadh $v05, $v00, $v00
  vmudl $v03, $v14, $v01.h0
  vmadm $v02, $v13, $v01.h0
  vmadn $v03, $v00, $v00
  jr $ra
  nop)");
}

// A multi-step sequence parks intermediates in VTEMP purely for the
// accumulator effect. VTEMP is shared with user code, so where the result
// register is overwritten later anyway it is used instead.
TEST_CASE("VectorOps - scratch uses the result register, not VTEMP",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test()
{
  vec32<$v05> posClip;
  vec16<$v15> guardBandScale;
  vec16<$v02> clipPlaneW = posClip * guardBandScale.xxxxxxxx;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudn $v02, $v06, $v15.e0
  vmadh $v02, $v05, $v15.e0
  jr $ra
  nop)");
}

TEST_CASE("VectorOps - scratch keeps VTEMP when the result is also a source",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test()
{
  vec32<$v05> big;
  vec16<$v02> out;
  out = big * out.xxxxxxxx;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  // $v02 is read by the vmadh, so it cannot take the scratch write
  REQUIRE(result.asm_ == R"(test:
  vmudn $v29, $v06, $v02.e0
  vmadh $v02, $v05, $v02.e0
  jr $ra
  nop)");
}

TEST_CASE("VectorOps - a live VTEMP survives an unrelated multiply",
          "[vectorOps]") {
  auto result = rspl::transpileSource(
      R"(function test()
{
  vec16<$v10> a;
  vec16<$v11> b;
  vec16<$v12> c;
  vec32<$v05> big;
  vec16<$v02> out;
  VTEMP = a * b;
  out = big * c.xxxxxxxx;
  a:sint += VTEMP;
})",
      {.rspqWrapper = false});

  REQUIRE(result.warn.empty());
  REQUIRE(result.asm_ == R"(test:
  vmudn $v29, $v10, $v11.v
  vmudn $v02, $v06, $v12.e0
  vmadh $v02, $v05, $v12.e0
  vadd $v10, $v10, $v29.v
  jr $ra
  nop)");
}

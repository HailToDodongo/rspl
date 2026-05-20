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
  REQUIRE(result.asm_ == R"(test:
  vaddc $v01, $v01, $v02.e0
  vadd $v01, $v01, $v02.e0
  vadd $v01, $v01, $v00.e0
  vaddc $v01, $v01, $v00.e0
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
  vmadl $v29, $v02, $v04.v
  vmadm $v29, $v01, $v04.v
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
  REQUIRE(result.asm_ == R"(test:
  vmudm $v06, $v01, $v04.v
  vmadh $v05, $v01, $v03.v
  vmadn $v06, $v00, $v00
  jr $ra
  nop)");
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

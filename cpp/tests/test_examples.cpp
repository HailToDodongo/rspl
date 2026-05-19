#include <catch2/catch_test_macros.hpp>
#include "pipeline.h"

#include <fstream>
#include <sstream>
#include <string>

static std::string readFile(const std::string &path) {
  std::ifstream f(path);
  REQUIRE(f.is_open());
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static std::string examplesPath(const std::string &rel) {
  return "src/tests/examples/" + rel;
}

// Transpile RSPL source, expect no errors and return the ASM.
static std::string transpile(const std::string &src, bool optimize,
                             bool debugInfo = false,
                             const std::string &sourceDir = ".") {
  rspl::TranspileConfig cfg;
  cfg.rspqWrapper = true;
  cfg.optimize = optimize;
  cfg.debugInfo = debugInfo;
  cfg.sourceDir = sourceDir;
  auto res = rspl::transpileSource(src, cfg);
  REQUIRE(res.warn.empty());
  return res.asm_;
}

static std::string transpileFile(const std::string &path, bool optimize,
                                 bool debugInfo = false) {
  // Set sourceDir to the directory of the file so includes resolve correctly
  auto lastSlash = path.rfind('/');
  std::string dir = (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : ".";
  return transpile(readFile(path), optimize, debugInfo, dir);
}

TEST_CASE("Examples - Squares 2D", "[examples]") {
  auto code = readFile(examplesPath("squares2d.rspl"));
  REQUIRE_NOTHROW(transpile(code, false));
}

TEST_CASE("Examples - 3D", "[examples]") {
  auto path = examplesPath("3d.rspl");
  auto expected = readFile(examplesPath("3d.S"));
  auto asm_ = transpileFile(path, true);
  REQUIRE(asm_ == expected);
}

TEST_CASE("Examples - Tiny3D", "[examples]") {
  auto path = examplesPath("t3d/rsp_tiny3d.rspl");
  auto expected = readFile(examplesPath("t3d/rsp_tiny3d.S"));
  auto asm_ = transpileFile(path, true, true);
  REQUIRE(asm_ == expected);
}

TEST_CASE("Examples - TinyPX", "[examples]") {
  auto path = examplesPath("t3d/rsp_tinypx.rspl");
  auto expected = readFile(examplesPath("t3d/rsp_tinypx.S"));
  auto asm_ = transpileFile(path, true, true);
  REQUIRE(asm_ == expected);
}

TEST_CASE("Examples - Mandelbrot", "[examples]") {
  auto code = readFile(examplesPath("mandelbrot.rspl"));
  REQUIRE_NOTHROW(transpile(code, false));
}

TEST_CASE("Examples - Matrix x Vector", "[examples]") {
  auto src = R"(
include "rsp_queue.inc"
state {
  vec32 VEC_SLOTS[20];
}

command<0> VecCmd_Transform(u32 vec_out, u32 mat_in)
{
  u32<$t0> trans_mtx = mat_in >> 16;
  trans_mtx &= 0xFF0;

  u32<$t1> trans_vec = mat_in & 0xFF0;
  u32<$t2> trans_out = vec_out & 0xFF0;

  trans_mtx += VEC_SLOTS;
  trans_vec += VEC_SLOTS;
  trans_out += VEC_SLOTS;

  vec32<$v01> mat0 = load(trans_mtx, 0).xyzwxyzw;
  vec32<$v03> mat1 = load(trans_mtx, 8).xyzwxyzw;
  vec32<$v05> mat2 = load(trans_mtx, 0x20).xyzwxyzw;
  vec32<$v07> mat3 = load(trans_mtx, 0x28).xyzwxyzw;

  vec32<$v09> vecIn = load(trans_vec);
  vec32<$v13> res;

  res = mat0  * vecIn.xxxxXXXX;
  res = mat1 +* vecIn.yyyyYYYY;
  res = mat2 +* vecIn.zzzzZZZZ;
  res = mat3 +* vecIn.wwwwWWWW;

  store(res, trans_out);
}

include "rsp_rdpq.inc"
)";
  auto asm_ = transpile(src, false);
  // Trim trailing whitespace to match JS test's .trimEnd()
  while (!asm_.empty() && (asm_.back() == '\n' || asm_.back() == ' '))
    asm_.pop_back();
  // Golden output from JS test
  REQUIRE(asm_ == R"(## Auto-generated file, transpiled with RSPL
#include <rsp_queue.inc>

.set noreorder
.set noat
.set nomacro

#undef zero
#undef at
#undef v0
#undef v1
#undef a0
#undef a1
#undef a2
#undef a3
#undef t0
#undef t1
#undef t2
#undef t3
#undef t4
#undef t5
#undef t6
#undef t7
#undef s0
#undef s1
#undef s2
#undef s3
#undef s4
#undef s5
#undef s6
#undef s7
#undef t8
#undef t9
#undef k0
#undef k1
#undef gp
#undef sp
#undef fp
#undef ra
.equ hex.$zero, 0
.equ hex.$at, 1
.equ hex.$v0, 2
.equ hex.$v1, 3
.equ hex.$a0, 4
.equ hex.$a1, 5
.equ hex.$a2, 6
.equ hex.$a3, 7
.equ hex.$t0, 8
.equ hex.$t1, 9
.equ hex.$t2, 10
.equ hex.$t3, 11
.equ hex.$t4, 12
.equ hex.$t5, 13
.equ hex.$t6, 14
.equ hex.$t7, 15
.equ hex.$s0, 16
.equ hex.$s1, 17
.equ hex.$s2, 18
.equ hex.$s3, 19
.equ hex.$s4, 20
.equ hex.$s5, 21
.equ hex.$s6, 22
.equ hex.$s7, 23
.equ hex.$t8, 24
.equ hex.$t9, 25
.equ hex.$k0, 26
.equ hex.$k1, 27
.equ hex.$gp, 28
.equ hex.$sp, 29
.equ hex.$fp, 30
.equ hex.$ra, 31
#define vco 0
#define vcc 1
#define vce 2

.data
  RSPQ_BeginOverlayHeader
    RSPQ_DefineCommand VecCmd_Transform, 8
  RSPQ_EndOverlayHeader

  RSPQ_BeginSavedState
    STATE_MEM_START:
    .align 4
    VEC_SLOTS: .ds.b 640
    STATE_MEM_END:
  RSPQ_EndSavedState

.text
OVERLAY_CODE_START:

VecCmd_Transform:
  srl $t0, $a1, 16
  andi $t0, $t0, 4080
  andi $t1, $a1, 4080
  andi $t2, $a0, 4080
  addiu $t0, $t0, %lo(VEC_SLOTS)
  addiu $t1, $t1, %lo(VEC_SLOTS)
  addiu $t2, $t2, %lo(VEC_SLOTS)
  ldv $v01, 0, 0, $t0
  ldv $v01, 8, 0, $t0
  ldv $v02, 0, 8, $t0
  ldv $v02, 8, 8, $t0
  ldv $v03, 0, 8, $t0
  ldv $v03, 8, 8, $t0
  ldv $v04, 0, 16, $t0
  ldv $v04, 8, 16, $t0
  ldv $v05, 0, 32, $t0
  ldv $v05, 8, 32, $t0
  ldv $v06, 0, 40, $t0
  ldv $v06, 8, 40, $t0
  ldv $v07, 0, 40, $t0
  ldv $v07, 8, 40, $t0
  ldv $v08, 0, 48, $t0
  ldv $v08, 8, 48, $t0
  lqv $v09, 0, 0, $t1
  lqv $v10, 0, 16, $t1
  vmudl $v29, $v02, $v10.h0
  vmadm $v29, $v01, $v10.h0
  vmadn $v14, $v02, $v09.h0
  vmadh $v13, $v01, $v09.h0
  vmadl $v29, $v04, $v10.h1
  vmadm $v29, $v03, $v10.h1
  vmadn $v14, $v04, $v09.h1
  vmadh $v13, $v03, $v09.h1
  vmadl $v29, $v06, $v10.h2
  vmadm $v29, $v05, $v10.h2
  vmadn $v14, $v06, $v09.h2
  vmadh $v13, $v05, $v09.h2
  vmadl $v29, $v08, $v10.h3
  vmadm $v29, $v07, $v10.h3
  vmadn $v14, $v08, $v09.h3
  vmadh $v13, $v07, $v09.h3
  sqv $v13, 0, 0, $t2
  sqv $v14, 0, 16, $t2
  j RSPQ_Loop
  nop

OVERLAY_CODE_END:

#define zero $0
#define v0 $2
#define v1 $3
#define a0 $4
#define a1 $5
#define a2 $6
#define a3 $7
#define t0 $8
#define t1 $9
#define t2 $10
#define t3 $11
#define t4 $12
#define t5 $13
#define t6 $14
#define t7 $15
#define s0 $16
#define s1 $17
#define s2 $18
#define s3 $19
#define s4 $20
#define s5 $21
#define s6 $22
#define s7 $23
#define t8 $24
#define t9 $25
#define k0 $26
#define k1 $27
#define gp $28
#define sp $29
#define fp $30
#define ra $31

.set at
.set macro
#include <rsp_rdpq.inc>)");
}

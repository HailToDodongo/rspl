#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "pipeline.h"

#include <string>

// 1:1 port of src/tests/magma/{magma,uniforms,attributes,shader}.test.js
// Source snippets are reproduced verbatim: the emitted LOAD_/PATCH_ labels
// embed the RSPL line number, so the line layout must match the JS exactly.

static std::string transpileMagma(const std::string &src) {
  rspl::TranspileConfig cfg;
  cfg.rspqWrapper = true;
  cfg.magma = true;
  auto res = rspl::transpileSource(src, cfg);
  REQUIRE(res.warn.empty());
  return res.asm_;
}

static void transpileMagmaThrows(const std::string &src) {
  rspl::TranspileConfig cfg;
  cfg.rspqWrapper = true;
  cfg.magma = true;
  rspl::transpileSource(src, cfg);
}

static void transpileNonMagma(const std::string &src) {
  rspl::TranspileConfig cfg;
  cfg.rspqWrapper = false;
  rspl::transpileSource(src, cfg);
}

static std::string section(const std::string &asm_, const std::string &begin,
                           const std::string &end) {
  auto idxBegin = asm_.find(begin);
  auto idxEnd = asm_.find(end);
  if (idxBegin == std::string::npos || idxEnd == std::string::npos) return "";
  return asm_.substr(idxBegin, idxEnd + end.size() - idxBegin);
}

static std::string getUniforms(const std::string &a) {
  return section(a, "MgBeginShaderUniforms", "MgEndShaderUniforms");
}
static std::string getAttributes(const std::string &a) {
  return section(a, "MgBeginVertexInput", "MgEndVertexInput");
}
static std::string getShader(const std::string &a) {
  auto idxEndUniforms = a.find("MgEndShaderUniform");
  auto idxBegin = a.find("MgBeginShader", idxEndUniforms);
  const std::string endKeyword = "MgEndShader";
  auto idxEnd = a.find(endKeyword, idxBegin);
  if (idxBegin == std::string::npos || idxEnd == std::string::npos) return "";
  return a.substr(idxBegin, idxEnd + endKeyword.size() - idxBegin);
}

static bool contains(const std::string &hay, const std::string &needle) {
  return hay.find(needle) != std::string::npos;
}

// ======================================================================
// magma.test.js — Magma mode
// ======================================================================

TEST_CASE("Magma mode - No RSPQ-Header in magma mode", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    shader testshader()
    {
    })RSPL");
  REQUIRE_FALSE(contains(asm_, "RSPQ_BeginOverlayHeader"));
  REQUIRE_FALSE(contains(asm_, "RSPQ_EndOverlayHeader"));
}

TEST_CASE("Magma mode - No saved state in magma mode", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    shader testshader()
    {
    })RSPL");
  REQUIRE_FALSE(contains(asm_, "RSPQ_BeginSavedState"));
  REQUIRE_FALSE(contains(asm_, "RSPQ_EndSavedState"));
  REQUIRE_FALSE(contains(asm_, "RSPQ_EmptySavedState"));
}

TEST_CASE("Magma mode - No sections in magma mode", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    shader testshader()
    {
    })RSPL");
  REQUIRE_FALSE(contains(asm_, ".data"));
  REQUIRE_FALSE(contains(asm_, ".text"));
}

TEST_CASE("Magma mode - Extern state", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    state
    {
      extern u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    })RSPL");
  REQUIRE(contains(asm_, "lw $t0, %lo(VALUE + 0)"));
  REQUIRE_FALSE(contains(asm_, "VALUE: .ds.b 4"));
}

TEST_CASE("Magma mode - Extern data", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    data
    {
      extern u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    })RSPL");
  REQUIRE(contains(asm_, "lw $t0, %lo(VALUE + 0)"));
  REQUIRE_FALSE(contains(asm_, "VALUE: .ds.b 4"));
}

TEST_CASE("Magma mode - Extern bss", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    bss
    {
      extern u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    })RSPL");
  REQUIRE(contains(asm_, "lw $t0, %lo(VALUE + 0)"));
  REQUIRE_FALSE(contains(asm_, "VALUE: .ds.b 4"));
}

TEST_CASE("Magma mode - Non-extern state in magma mode", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    state
    {
      u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Only extern states are allowed when compiling for magma!"));
}

TEST_CASE("Magma mode - Non-extern data in magma mode", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    data
    {
      u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Only extern states are allowed when compiling for magma!"));
}

TEST_CASE("Magma mode - Non-extern bss in magma mode", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    bss
    {
      u32 VALUE;
    }

    shader testshader()
    {
      u32<$t0> value = load(VALUE);
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Only extern states are allowed when compiling for magma!"));
}

// ======================================================================
// uniforms.test.js — Uniforms
// ======================================================================

TEST_CASE("Uniforms - Single uniform", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    uniform<0> UNIFORM0
    {
        u32 VALUE0;
    }

    shader testshader()
    {
    })RSPL");
  REQUIRE(getUniforms(asm_) ==
          "MgBeginShaderUniforms\n"
          "  MgBeginUniform UNIFORM0, 0\n"
          "    .align 2\n"
          "    VALUE0: .ds.b 4\n"
          "  MgEndUniform\n"
          "\n"
          "MgEndShaderUniforms");
}

TEST_CASE("Uniforms - Multiple values", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    uniform<0> UNIFORM0
    {
        vec16 POSITIONS[2];
        u32 VALUE0;
        u32 VALUES[4];
    }

    shader testshader()
    {
    })RSPL");
  REQUIRE(getUniforms(asm_) ==
          "MgBeginShaderUniforms\n"
          "  MgBeginUniform UNIFORM0, 0\n"
          "    .align 4\n"
          "    POSITIONS: .ds.b 32\n"
          "    .align 2\n"
          "    VALUE0: .ds.b 4\n"
          "    .align 2\n"
          "    VALUES: .ds.b 16\n"
          "  MgEndUniform\n"
          "\n"
          "MgEndShaderUniforms");
}

TEST_CASE("Uniforms - Multiple uniforms", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    uniform<0> UNIFORM0
    {
        u32 VALUE0;
    }

    uniform<1> UNIFORM1
    {
        s16 POSITION[3];
    }

    shader testshader()
    {
    })RSPL");
  REQUIRE(getUniforms(asm_) ==
          "MgBeginShaderUniforms\n"
          "  MgBeginUniform UNIFORM0, 0\n"
          "    .align 2\n"
          "    VALUE0: .ds.b 4\n"
          "  MgEndUniform\n"
          "\n"
          "  MgBeginUniform UNIFORM1, 1\n"
          "    .align 1\n"
          "    POSITION: .ds.b 6\n"
          "  MgEndUniform\n"
          "\n"
          "MgEndShaderUniforms");
}

TEST_CASE("Uniforms - Arbitrary binding numbers", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    uniform<748> UNIFORM0
    {
        u32 VALUE0;
    }

    uniform<34> UNIFORM1
    {
        u32 VALUE1;
    }

    shader testshader()
    {
    })RSPL");
  REQUIRE(getUniforms(asm_) ==
          "MgBeginShaderUniforms\n"
          "  MgBeginUniform UNIFORM0, 748\n"
          "    .align 2\n"
          "    VALUE0: .ds.b 4\n"
          "  MgEndUniform\n"
          "\n"
          "  MgBeginUniform UNIFORM1, 34\n"
          "    .align 2\n"
          "    VALUE1: .ds.b 4\n"
          "  MgEndUniform\n"
          "\n"
          "MgEndShaderUniforms");
}

TEST_CASE("Uniforms - Omitted binding numbers", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    uniform<1> UNIFORM0
    {
        u32 VALUE0;
    }

    uniform UNIFORM1
    {
        u32 VALUE1;
    }

    uniform UNIFORM2
    {
        u32 VALUE2;
    }

    shader testshader()
    {
    })RSPL");
  REQUIRE(getUniforms(asm_) ==
          "MgBeginShaderUniforms\n"
          "  MgBeginUniform UNIFORM0, 1\n"
          "    .align 2\n"
          "    VALUE0: .ds.b 4\n"
          "  MgEndUniform\n"
          "\n"
          "  MgBeginUniform UNIFORM1, 2\n"
          "    .align 2\n"
          "    VALUE1: .ds.b 4\n"
          "  MgEndUniform\n"
          "\n"
          "  MgBeginUniform UNIFORM2, 3\n"
          "    .align 2\n"
          "    VALUE2: .ds.b 4\n"
          "  MgEndUniform\n"
          "\n"
          "MgEndShaderUniforms");
}

TEST_CASE("Uniforms - Negative binding number", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    uniform<-3> UNIFORM0
    {
        u32 VALUE0;
    }

    shader testshader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Uniform binding number must be in [0, 2^32)!"));
}

TEST_CASE("Uniforms - Very large binding number", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    uniform<69347592054634> UNIFORM0
    {
        u32 VALUE0;
    }

    shader testshader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Uniform binding number must be in [0, 2^32)!"));
}

TEST_CASE("Uniforms - Invalid binding number", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    uniform<wrong> UNIFORM0
    {
        u32 VALUE0;
    }

    shader testshader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring("Syntax error at line 2"));
}

TEST_CASE("Uniforms - Empty uniform", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    uniform<0> UNIFORM0
    {
    }

    shader testshader()
    {
    })RSPL");
  REQUIRE(getUniforms(asm_) ==
          "MgBeginShaderUniforms\n"
          "  MgBeginUniform UNIFORM0, 0\n"
          "  MgEndUniform\n"
          "\n"
          "MgEndShaderUniforms");
}

TEST_CASE("Uniforms - Extern value", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    uniform<0> UNIFORM0
    {
      u32 VALUE0;
      extern u32 VALUE1;
    }

    shader testshader()
    {
    })RSPL");
  REQUIRE(getUniforms(asm_) ==
          "MgBeginShaderUniforms\n"
          "  MgBeginUniform UNIFORM0, 0\n"
          "    .align 2\n"
          "    VALUE0: .ds.b 4\n"
          "  MgEndUniform\n"
          "\n"
          "MgEndShaderUniforms");
}

TEST_CASE("Uniforms - No uniforms", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    shader testshader()
    {
    })RSPL");
  REQUIRE(getUniforms(asm_) == "MgBeginShaderUniforms\nMgEndShaderUniforms");
}

TEST_CASE("Uniforms - Uniform in non-magma mode", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileNonMagma(R"RSPL(
    uniform<0> UNIFORM0
    {
        u32 VALUE0;
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Uniforms are only allowed when compiling for magma (pass "
          "'--magma' on the command line)!"));
}

// ======================================================================
// attributes.test.js — Attributes
// ======================================================================

TEST_CASE("Attributes - Unused attribute", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0;

    shader testshader()
    {
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 0\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
}

TEST_CASE("Attributes - Scalar loader", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0;

    shader testshader()
    {
      u32<$t0> vtx;
      u32<$t1> attr0;
      @AttrLoader("ATTRIBUTE0") attr0 = load(vtx);
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 0\n"
          "    MgVertexAttributeLoaders LOAD_ATTRIBUTE08\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE08: lw $t1, 0($t0)"));
}

TEST_CASE("Attributes - Vector loader", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> vec16 ATTRIBUTE0;

    shader testshader()
    {
      u32<$t0> vtx;
      vec16<$v01> attr0;
      @AttrLoader("ATTRIBUTE0") attr0 = load(vtx);
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 0\n"
          "    MgVertexAttributeLoaders LOAD_ATTRIBUTE08\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE08: lqv $v01, 0, 0, $t0"));
}

TEST_CASE("Attributes - Loader on declaration", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0;

    shader testshader()
    {
      u32<$t0> vtx;
      @AttrLoader("ATTRIBUTE0") u32<$t1> attr0 = load(vtx);
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 0\n"
          "    MgVertexAttributeLoaders LOAD_ATTRIBUTE07\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE07: lw $t1, 0($t0)"));
}

TEST_CASE("Attributes - Multiple loaders", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0;

    shader testshader()
    {
      u32<$t0> vtx;
      @AttrLoader("ATTRIBUTE0") u32<$t1> attr0 = load(vtx);
      vec16<$v01> attr1;
      @AttrLoader("ATTRIBUTE0") attr1.xy = load(vtx).xy;
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 0\n"
          "    MgVertexAttributeLoaders LOAD_ATTRIBUTE07, LOAD_ATTRIBUTE09\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE07: lw $t1, 0($t0)"));
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE09: llv $v01, 0, 0, $t0"));
}

TEST_CASE("Attributes - Loader (non-string value)", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
      u32<$t0> vtx;
      vec16<$v01> attr0;
      @AttrLoader(10) attr0 = load(vtx);
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "line 8: Annotation 'AttrLoader' expects a string value!"));
}

TEST_CASE("Attributes - Loader (empty string value)", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
      u32<$t0> vtx;
      vec16<$v01> attr0;
      @AttrLoader("") attr0 = load(vtx);
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "line 8: Annotation 'AttrLoader' expects a non-empty string value!"));
}

TEST_CASE("Attributes - Optional attribute", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 1\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
}

TEST_CASE("Attributes - Patch", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
      u32<$t0> a;
      @AttrPatch("ATTRIBUTE0:nop") a = 1;
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 1\n"
          "    MgBeginVertexAttributePatch PATCH_ATTRIBUTE07\n"
          "      nop\n"
          "    MgEndVertexAttributePatch\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "PATCH_ATTRIBUTE07: addiu $t0, $zero, 1"));
}

TEST_CASE("Attributes - Patch (missing replacement)", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
      u32<$t0> a;
      @AttrPatch("ATTRIBUTE0:") a = 1;
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 1\n"
          "    MgBeginVertexAttributePatch PATCH_ATTRIBUTE07\n"
          "      nop\n"
          "    MgEndVertexAttributePatch\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "PATCH_ATTRIBUTE07: addiu $t0, $zero, 1"));
}

TEST_CASE("Attributes - Patch (missing colon)", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
      u32<$t0> a;
      @AttrPatch("ATTRIBUTE0") a = 1;
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 1\n"
          "    MgBeginVertexAttributePatch PATCH_ATTRIBUTE07\n"
          "      nop\n"
          "    MgEndVertexAttributePatch\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "PATCH_ATTRIBUTE07: addiu $t0, $zero, 1"));
}

TEST_CASE("Attributes - Patch (non-string value)", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
      u32<$t0> a;
      @AttrPatch(1) a = 1;
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "line 7: Annotation 'AttrPatch' expects a string value!"));
}

TEST_CASE("Attributes - Patch (empty string value)", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
      u32<$t0> a;
      @AttrPatch("") a = 1;
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "line 7: Annotation 'AttrPatch' expects a non-empty string value!"));
}

TEST_CASE("Attributes - Multiple patches", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0?;

    shader testshader()
    {
      u32<$t0> a, b;
      @AttrPatch("ATTRIBUTE0:nop") a = 1;
      @AttrPatch("ATTRIBUTE0:addiu $t1, $zero, $zero") b = a + a;
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 1\n"
          "    MgBeginVertexAttributePatch PATCH_ATTRIBUTE07\n"
          "      nop\n"
          "    MgEndVertexAttributePatch\n"
          "    MgBeginVertexAttributePatch PATCH_ATTRIBUTE08\n"
          "      addiu $t1, $zero, $zero\n"
          "    MgEndVertexAttributePatch\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "PATCH_ATTRIBUTE07: addiu $t0, $zero, 1"));
  REQUIRE(contains(asm_, "PATCH_ATTRIBUTE08: addu $t1, $t0, $t0"));
}

TEST_CASE("Attributes - Multiple attributes", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0;
    attribute<1> u32 ATTRIBUTE1?;
    attribute<2> u32 ATTRIBUTE2?;

    shader testshader()
    {
      u32<$s0> vtx;
      u32<$t0> a, b, c;
      @AttrLoader("ATTRIBUTE0") a = load(vtx);
      @AttrLoader("ATTRIBUTE1") b = load(vtx);
      @AttrLoader("ATTRIBUTE2") c = load(vtx);
      @AttrPatch("ATTRIBUTE1:nop") a = 1;
      @AttrLoader("ATTRIBUTE1") b = load(vtx);
      @AttrLoader("ATTRIBUTE2") c = load(vtx);
      @AttrPatch("ATTRIBUTE1:nop") b = a + a;
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 0, 0\n"
          "    MgVertexAttributeLoaders LOAD_ATTRIBUTE010\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "  MgBeginVertexAttribute 1, 1\n"
          "    MgVertexAttributeLoaders LOAD_ATTRIBUTE111, LOAD_ATTRIBUTE114\n"
          "    MgBeginVertexAttributePatch PATCH_ATTRIBUTE113\n"
          "      nop\n"
          "    MgEndVertexAttributePatch\n"
          "    MgBeginVertexAttributePatch PATCH_ATTRIBUTE116\n"
          "      nop\n"
          "    MgEndVertexAttributePatch\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "  MgBeginVertexAttribute 2, 1\n"
          "    MgVertexAttributeLoaders LOAD_ATTRIBUTE212, LOAD_ATTRIBUTE215\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE010: lw $t0, 0($s0)"));
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE111: lw $t1, 0($s0)"));
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE114: lw $t1, 0($s0)"));
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE212: lw $t2, 0($s0)"));
  REQUIRE(contains(asm_, "LOAD_ATTRIBUTE215: lw $t2, 0($s0)"));
  REQUIRE(contains(asm_, "PATCH_ATTRIBUTE113: addiu $t0, $zero, 1"));
  REQUIRE(contains(asm_, "PATCH_ATTRIBUTE116: addu $t1, $t0, $t0"));
}

TEST_CASE("Attributes - Arbitrary input numbers", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<65> u32 ATTRIBUTE0;
    attribute<6> u32 ATTRIBUTE1;

    shader testshader()
    {
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 65, 0\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "  MgBeginVertexAttribute 6, 0\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
}

TEST_CASE("Attributes - Omitted input numbers", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    attribute<1> u32 ATTRIBUTE0;
    attribute u32 ATTRIBUTE1;
    attribute u32 ATTRIBUTE2;

    shader testshader()
    {
    })RSPL");
  REQUIRE(getAttributes(asm_) ==
          "MgBeginVertexInput\n"
          "  MgBeginVertexAttribute 1, 0\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "  MgBeginVertexAttribute 2, 0\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "  MgBeginVertexAttribute 3, 0\n"
          "  MgEndVertexAttribute\n"
          "\n"
          "MgEndVertexInput");
}

TEST_CASE("Attributes - Negative input number", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    attribute<-2> u32 ATTRIBUTE0;

    shader testshader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Attribute input number must be in [0, 2^32)!"));
}

// Same name as the previous test in the JS suite; kept distinct for Catch2.
TEST_CASE("Attributes - Negative input number (very large)", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    attribute<69347592054634> u32 ATTRIBUTE0;

    shader testshader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Attribute input number must be in [0, 2^32)!"));
}

TEST_CASE("Attributes - Invalid input number", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    attribute<wrong> u32 ATTRIBUTE0;

    shader testshader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring("Syntax error at line 2"));
}

TEST_CASE("Attributes - No attributes", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    shader testshader()
    {
    })RSPL");
  REQUIRE(getAttributes(asm_) == "MgBeginVertexInput\nMgEndVertexInput");
}

TEST_CASE("Attributes - Attribute in non-magma mode", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileNonMagma(R"RSPL(
    attribute<0> u32 ATTRIBUTE0;
    )RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Attributes are only allowed when compiling for magma (pass "
          "'--magma' on the command line)!"));
}

// ======================================================================
// shader.test.js — Shaders
// ======================================================================

TEST_CASE("Shaders - Empty shader", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    shader testshader()
    {
    })RSPL");
  REQUIRE(getShader(asm_) ==
          "MgBeginShader\n"
          "  j RSPQ_Loop\n"
          "  nop\n"
          "\n"
          "MgEndShader");
}

TEST_CASE("Shaders - Simple shader", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    shader testshader()
    {
      u32<$a0> ptr;
      u32<$t0> value = 0x100;
      store(value, ptr);
    })RSPL");
  REQUIRE(getShader(asm_) ==
          "MgBeginShader\n"
          "  addiu $t0, $zero, 256\n"
          "  sw $t0, ($a0)\n"
          "  j RSPQ_Loop\n"
          "  nop\n"
          "\n"
          "MgEndShader");
}

TEST_CASE("Shaders - Function before shader", "[magma]") {
  auto asm_ = transpileMagma(R"RSPL(
    function test_function(u32<$s0> ptr)
    {
      u32<$t0> value = 1;
      store(value, ptr);
    }

    shader testshader()
    {
      u32<$s0> ptr = 0x100;
      test_function(ptr);
    })RSPL");
  REQUIRE(getShader(asm_) ==
          "MgBeginShader\n"
          "  addiu $s0, $zero, 256\n"
          "  jal test_function\n"
          "  nop\n"
          "  j RSPQ_Loop\n"
          "  nop\n"
          "test_function:\n"
          "  addiu $t0, $zero, 1\n"
          "  sw $t0, ($s0)\n"
          "  jr $ra\n"
          "  nop\n"
          "\n"
          "MgEndShader");
}

TEST_CASE("Shaders - Arguments in shader", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    shader test_shader(u32 arg)
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Shaders must not specify arguments!"));
}

TEST_CASE("Shaders - Result type in shader", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    shader<$t0> test_shader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Shaders must not specify a result-type (use 'shader' without "
          "`< >`)!"));
}

TEST_CASE("Shaders - Missing shader", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    function test_missing_shader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Exactly one shader must be defined when compiling for magma (use "
          "'shader')!"));
}

TEST_CASE("Shaders - Multiple shaders", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    shader test_shader1()
    {
    }

    shader test_shader2()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring("A shader has already been defined!"));
}

TEST_CASE("Shaders - Command in magma mode", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileMagmaThrows(R"RSPL(
    command<0> test_command()
    {
    }

    shader testshader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Commands must not be defined when compiling for magma (define a "
          "'shader' instead)!"));
}

TEST_CASE("Shaders - Shader in non-magma mode", "[magma]") {
  REQUIRE_THROWS_WITH(
      transpileNonMagma(R"RSPL(
    shader testshader()
    {
    })RSPL"),
      Catch::Matchers::ContainsSubstring(
          "Shaders are only allowed when compiling for magma (pass '--magma' "
          "on the command line)!"));
}

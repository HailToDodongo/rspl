#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

// --- Type enums (replaces string-based type checks in hot paths) ----------

enum class TypeClass : uint8_t {
  Unknown = 0,  // must be 0 so VarDef{} zero-inits to Unknown, not Vec32
  Vec32,
  Vec16,
  U32,
  U16,
  U8,
  S32,
  S16,
  S8,
  Sfract,
  Ufract,
  Sint,
  Uint,
};

enum class CastType : uint8_t {
  None,
  Sfract,
  Ufract,
  Sint,
  Uint,
  S8,
  S16
};

enum class FuncType : uint8_t { Function, Command, Macro, Shader };

enum class ArgType : uint8_t { Var, Num, String };

// --- Conversion: string <-> enum ----------------------------------------

TypeClass toTypeClass(const std::string &s);
std::string toString(TypeClass tc);
CastType toCastType(const std::string &s);
std::string toString(CastType ct);
FuncType toFuncType(const std::string &s);
std::string toString(FuncType ft);
ArgType toArgType(const std::string &s);
std::string toString(ArgType at);

// --- Type checks (now on enums, single-cycle) ---------------------------

inline bool isVecType(TypeClass tc) {
  return tc == TypeClass::Vec32 || tc == TypeClass::Vec16;
}
inline bool isTwoRegType(TypeClass tc) { return tc == TypeClass::Vec32; }
inline bool isSigned(TypeClass tc) {
  return tc == TypeClass::S32 || tc == TypeClass::S16 || tc == TypeClass::S8 ||
         tc == TypeClass::Sint || tc == TypeClass::Sfract;
}

// --- Backward-compat helpers for places that still use strings -----------

// These overloads exist so call sites that currently pass strings
// work during the transition.
inline bool isVecType(const std::string &s) { return isVecType(toTypeClass(s)); }
inline bool isTwoRegType(const std::string &s) { return s == "vec32"; }

// --- Type size / alignment / reg count ----------------------------------

struct TypeAsmDef {
  std::string type; // "byte", "half", "word"
  int count;
};

extern const std::unordered_map<std::string, int> TYPE_SIZE;
extern const std::unordered_map<std::string, int> TYPE_ALIGNMENT;
extern const std::unordered_map<std::string, int> TYPE_REG_COUNT;
extern const std::unordered_map<std::string, TypeAsmDef> TYPE_ASM_DEF;

// --- Type lists ---------------------------------------------------------

extern const std::vector<std::string> SCALAR_TYPES;
extern const std::vector<std::string> VEC_CASTS;

// --- Misc helpers -------------------------------------------------------

inline std::string toHex(int64_t val, int pad = 2) {
  char buf[32];
  snprintf(buf, sizeof(buf), "0x%0*llX", pad,
           static_cast<long long>(val));
  return buf;
}

inline bool u32InS16Range(uint32_t valueU32) {
  return valueU32 <= 0x7FFF || valueU32 >= 0xFFFF8000;
}

inline bool u32InU16Range(uint32_t valueU32) { return valueU32 <= 0xFFFF; }

inline uint32_t f32ToFP32(float valueF32) {
  return static_cast<uint32_t>(static_cast<int32_t>(valueF32 * (1 << 16)));
}

} // namespace rspl
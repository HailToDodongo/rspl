#include "types.h"

#include <stdexcept>

namespace rspl {

// --- string -> enum conversions -----------------------------------------

TypeClass toTypeClass(const std::string &s) {
  if (s == "vec32") return TypeClass::Vec32;
  if (s == "vec16") return TypeClass::Vec16;
  if (s == "u32") return TypeClass::U32;
  if (s == "u16") return TypeClass::U16;
  if (s == "u8") return TypeClass::U8;
  if (s == "s32") return TypeClass::S32;
  if (s == "s16") return TypeClass::S16;
  if (s == "s8") return TypeClass::S8;
  if (s == "sfract") return TypeClass::Sfract;
  if (s == "ufract") return TypeClass::Ufract;
  if (s == "sint") return TypeClass::Sint;
  if (s == "uint") return TypeClass::Uint;
  if (s.empty()) return TypeClass::Unknown;
  throw std::runtime_error("Unknown type class: " + s);
}

std::string toString(TypeClass tc) {
  switch (tc) {
  case TypeClass::Vec32:   return "vec32";
  case TypeClass::Vec16:   return "vec16";
  case TypeClass::U32:     return "u32";
  case TypeClass::U16:     return "u16";
  case TypeClass::U8:      return "u8";
  case TypeClass::S32:     return "s32";
  case TypeClass::S16:     return "s16";
  case TypeClass::S8:      return "s8";
  case TypeClass::Sfract:  return "sfract";
  case TypeClass::Ufract:  return "ufract";
  case TypeClass::Sint:    return "sint";
  case TypeClass::Uint:    return "uint";
  case TypeClass::Unknown: return "";
  }
  return "";
}

CastType toCastType(const std::string &s) {
  if (s.empty()) return CastType::None;
  if (s == "sfract") return CastType::Sfract;
  if (s == "ufract") return CastType::Ufract;
  if (s == "sint") return CastType::Sint;
  if (s == "uint") return CastType::Uint;
  if (s == "s8") return CastType::S8;
  if (s == "s16") return CastType::S16;
  // Scalar type casts (u32, s16, etc.) are not vector cast types
  if (s == "u32" || s == "u16" || s == "u8" ||
      s == "s32" || s == "s16" || s == "s8")
    return CastType::None;
  throw std::runtime_error("Unknown cast type: " + s);
}

std::string toString(CastType ct) {
  switch (ct) {
  case CastType::None:    return "";
  case CastType::Sfract:  return "sfract";
  case CastType::Ufract:  return "ufract";
  case CastType::Sint:    return "sint";
  case CastType::Uint:    return "uint";
  case CastType::S8:      return "s8";
  case CastType::S16:     return "s16";
  }
  return "";
}

FuncType toFuncType(const std::string &s) {
  if (s == "function") return FuncType::Function;
  if (s == "command")  return FuncType::Command;
  if (s == "macro")    return FuncType::Macro;
  throw std::runtime_error("Unknown function type: " + s);
}

std::string toString(FuncType ft) {
  switch (ft) {
  case FuncType::Function: return "function";
  case FuncType::Command:  return "command";
  case FuncType::Macro:    return "macro";
  }
  return "function";
}

ArgType toArgType(const std::string &s) {
  if (s == "var")  return ArgType::Var;
  if (s == "num")  return ArgType::Num;
  if (s == "string") return ArgType::String;
  throw std::runtime_error("Unknown arg type: " + s);
}

std::string toString(ArgType at) {
  switch (at) {
  case ArgType::Var:    return "var";
  case ArgType::Num:    return "num";
  case ArgType::String: return "string";
  }
  return "var";
}

const std::unordered_map<std::string, int> TYPE_SIZE = {
    {"s8", 1},   {"u8", 1},   {"s16", 2},   {"u16", 2},
    {"s32", 4},  {"u32", 4},  {"vec16", 16}, {"vec32", 32},
};

const std::unordered_map<std::string, int> TYPE_ALIGNMENT = {
    {"s8", 0}, {"u8", 0},   {"s16", 1},   {"u16", 1},
    {"s32", 2}, {"u32", 2}, {"vec16", 4}, {"vec32", 4},
};

const std::unordered_map<std::string, int> TYPE_REG_COUNT = {
    {"s8", 1}, {"u8", 1},   {"s16", 1},  {"u16", 1},
    {"s32", 1}, {"u32", 1}, {"vec16", 1}, {"vec32", 2},
};

const std::unordered_map<std::string, TypeAsmDef> TYPE_ASM_DEF = {
    {"s8",   {"byte", 1}},
    {"u8",   {"byte", 1}},
    {"s16",  {"half", 1}},
    {"u16",  {"half", 1}},
    {"s32",  {"word", 1}},
    {"u32",  {"word", 1}},
    {"vec16", {"half", 8}},
    {"vec32", {"half", 16}},
};

const std::vector<std::string> SCALAR_TYPES = {
    "s8", "u8", "s16", "u16", "s32", "u32"};

const std::vector<std::string> VEC_CASTS = {"uint", "sint", "ufract", "sfract"};

} // namespace rspl

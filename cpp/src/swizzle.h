#pragma once

#include <string>
#include <unordered_map>

namespace rspl {

// Swizzle string -> MIPS element suffix (.e0, .h0, etc.)
extern const std::unordered_map<std::string, std::string> SWIZZLE_MAP;

// Swizzle lane character -> byte offset index
extern const std::unordered_map<char, int> SWIZZLE_SCALAR_IDX;

// Power-of-two values -> vector register + swizzle reference
struct Pow2SwizzleRef {
  std::string reg;    // e.g. "$v30" (VSHIFT)
  std::string swizzle; // e.g. "x"
};
extern const std::unordered_map<int64_t, Pow2SwizzleRef> POW2_SWIZZLE_VAR;

// Check if swizzle is a single-lane access (.x to .W)
inline bool isScalarSwizzle(const std::string &s) { return s.size() == 1; }

} // namespace rspl

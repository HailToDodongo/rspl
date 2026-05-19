#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

// --- Type size / alignment / reg count --------------------------------

struct TypeAsmDef {
  std::string type; // "byte", "half", "word"
  int count;
};

extern const std::unordered_map<std::string, int> TYPE_SIZE;
extern const std::unordered_map<std::string, int> TYPE_ALIGNMENT;
extern const std::unordered_map<std::string, int> TYPE_REG_COUNT;
extern const std::unordered_map<std::string, TypeAsmDef> TYPE_ASM_DEF;

// --- Type lists -------------------------------------------------------

extern const std::vector<std::string> SCALAR_TYPES;
extern const std::vector<std::string> VEC_CASTS;

// --- Type helpers -----------------------------------------------------

inline bool isTwoRegType(const std::string &type) { return type == "vec32"; }

inline bool isVecType(const std::string &type) {
  return type.size() >= 3 && type[0] == 'v' && type[1] == 'e' &&
         type[2] == 'c';
}

inline bool isSigned(const std::string &type) {
  return !type.empty() && type[0] == 's';
}

inline std::string toHex(int64_t val, int pad = 2) {
  char buf[32];
  snprintf(buf, sizeof(buf), "0x%0*llX", pad,
           static_cast<long long>(val));
  return buf;
}

inline bool u32InS16Range(uint32_t valueU32) {
  return valueU32 <= 0x7FFF || valueU32 >= 0xFFFF8000;
}

inline bool u32InU16Range(uint32_t valueU32) {
  return valueU32 <= 0xFFFF;
}

inline uint32_t f32ToFP32(float valueF32) {
  return static_cast<uint32_t>(static_cast<int32_t>(valueF32 * (1 << 16)));
}

} // namespace rspl

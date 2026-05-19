#include "types.h"

namespace rspl {

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

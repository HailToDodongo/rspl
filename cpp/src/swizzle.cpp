#include "swizzle.h"

namespace rspl {

const std::unordered_map<std::string, std::string> SWIZZLE_MAP = {
    {"", ".v"},       {"xyzwXYZW", ".v"},  {"xxzzXXZZ", ".q0"},
    {"yywwYYWW", ".q1"},  {"xxxxXXXX", ".h0"},  {"yyyyYYYY", ".h1"},
    {"zzzzZZZZ", ".h2"},  {"wwwwWWWW", ".h3"},  {"xxxxxxxx", ".e0"},
    {"yyyyyyyy", ".e1"},  {"zzzzzzzz", ".e2"},  {"wwwwwwww", ".e3"},
    {"XXXXXXXX", ".e4"},  {"YYYYYYYY", ".e5"},  {"ZZZZZZZZ", ".e6"},
    {"WWWWWWWW", ".e7"},  {"x", ".e0"},         {"y", ".e1"},
    {"z", ".e2"},         {"w", ".e3"},         {"X", ".e4"},
    {"Y", ".e5"},         {"Z", ".e6"},         {"W", ".e7"},
};

const std::unordered_map<char, int> SWIZZLE_SCALAR_IDX = {
    {'x', 0}, {'y', 1}, {'z', 2}, {'w', 3},
    {'X', 4}, {'Y', 5}, {'Z', 6}, {'W', 7},
};

const std::unordered_map<int64_t, Pow2SwizzleRef> POW2_SWIZZLE_VAR = {
    {0,     {"$v00", "x"}}, {1,     {"$v30", "W"}},
    {2,     {"$v30", "Z"}}, {4,     {"$v30", "Y"}},
    {8,     {"$v30", "X"}}, {16,    {"$v30", "w"}},
    {32,    {"$v30", "z"}}, {64,    {"$v30", "y"}},
    {128,   {"$v30", "x"}}, {256,   {"$v31", "W"}},
    {512,   {"$v31", "Z"}}, {1024,  {"$v31", "Y"}},
    {2048,  {"$v31", "X"}}, {4096,  {"$v31", "w"}},
    {8192,  {"$v31", "z"}}, {16384, {"$v31", "y"}},
    {32768, {"$v31", "x"}},
};

} // namespace rspl

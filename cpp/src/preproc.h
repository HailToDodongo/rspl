#pragma once

#include <string>
#include <unordered_map>

namespace rspl {

struct DefineEntry {
  std::string name;
  std::string value;
};

/// Strip C-style comments (// and /* */) from source
std::string stripComments(const std::string &source);

/// Preprocess with C-style #define, #ifdef, #ifndef, #include, #undef.
/// @param defines  map of name->value for predefined defines (modified in-place)
std::string preprocess(const std::string &src,
                       std::unordered_map<std::string, DefineEntry> &defines,
                       const std::string &sourceDir = ".");

/// Convenience: stripComments + preprocess
std::string preprocFull(const std::string &src,
                        std::unordered_map<std::string, DefineEntry> &defines,
                        const std::string &sourceDir = ".");

} // namespace rspl

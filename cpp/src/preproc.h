#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

struct DefineEntry {
  std::string name;
  std::string value;
};

/// Strip C-style comments (// and /* */) from source
std::string stripComments(const std::string &source);

/// Preprocess with C-style #define, #ifdef, #ifndef, #include, #undef.
/// @param defines  map of name->value for predefined defines (modified in-place)
/// @param defineOrder  if non-null, records defines in source order
std::string preprocess(const std::string &src,
                       std::unordered_map<std::string, DefineEntry> &defines,
                       const std::string &sourceDir = ".",
                       std::vector<DefineEntry> *defineOrder = nullptr);

/// Convenience: stripComments + preprocess
std::string preprocFull(const std::string &src,
                        std::unordered_map<std::string, DefineEntry> &defines,
                        const std::string &sourceDir = ".",
                        std::vector<DefineEntry> *defineOrder = nullptr);

} // namespace rspl

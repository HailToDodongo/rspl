#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

struct DefineEntry {
  std::string name;
  std::string value;
};

/// Where a line of the preprocessed text came from. `#include` splices whole
/// files in, so a preprocessed line number is not the number the author sees
/// in their editor; this maps it back.
struct SourceLoc {
  std::string file; // empty: the file being compiled
  int line = 0;     // 1-based line within `file`
};

/// Strip C-style comments (// and /* */) from source
std::string stripComments(const std::string &source);

/// Preprocess with C-style #define, #ifdef, #ifndef, #include, #undef.
/// @param defines  map of name->value for predefined defines (modified in-place)
/// @param defineOrder  if non-null, records defines in source order
std::string preprocess(const std::string &src,
                       std::unordered_map<std::string, DefineEntry> &defines,
                       const std::string &sourceDir = ".",
                       std::vector<DefineEntry> *defineOrder = nullptr,
                       std::vector<SourceLoc> *origins = nullptr,
                       const std::string &fileName = {});

/// Convenience: stripComments + preprocess
std::string preprocFull(const std::string &src,
                        std::unordered_map<std::string, DefineEntry> &defines,
                        const std::string &sourceDir = ".",
                        std::vector<DefineEntry> *defineOrder = nullptr,
                        std::vector<SourceLoc> *origins = nullptr,
                        const std::string &fileName = {});

} // namespace rspl

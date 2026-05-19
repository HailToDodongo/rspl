#pragma once

#include <string>

namespace rspl {

struct TranspileConfig {
  bool rspqWrapper = true;
  bool optimize = false;
  bool debugInfo = false;
  int optimizeTime = 5000;
};

struct TranspileResult {
  std::string asm_;
  std::string warn;
};

/// Run the full transpile pipeline on a JSON AST from the JS parser.
/// Returns 0 on success, non-zero on error.
int runPipeline(const std::string &astJson, bool rspqWrapper = true,
                bool optimize = false);

/// Transpile an RSPL source string to assembly.
/// Handles the JS parser subprocess internally.
/// Throws std::runtime_error on parse/compile errors.
TranspileResult transpileSource(const std::string &source,
                                const TranspileConfig &config = {});

} // namespace rspl

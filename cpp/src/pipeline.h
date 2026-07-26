#pragma once

#include <string>

namespace rspl {

struct TranspileConfig {
  bool rspqWrapper = true;
  bool optimize = false;
  bool debugInfo = false;
  bool reorder = false;
  bool magma = false;       // emit a magma shader instead of an RSPQ overlay
  int optimizeTime = 30000; // ms, default 30s matching CLI
  int optWorkers = 0;       // 0 = auto (hw threads - 1)
  std::string sourceDir = ".";
};

struct TranspileResult {
  std::string asm_;
  std::string warn;
  std::string info;
  int sizeDMEM = 0;
  int sizeIMEM = 0;
};

/// Run the full transpile pipeline on a JSON AST from the JS parser.
/// Returns the transpile result. Throws std::runtime_error on errors.
TranspileResult runPipeline(const std::string &astJson,
                            const TranspileConfig &config = {});

/// Transpile an RSPL source string to assembly.
/// Handles the JS parser subprocess internally.
/// Throws std::runtime_error on parse/compile errors.
TranspileResult transpileSource(const std::string &source,
                                const TranspileConfig &config = {});

} // namespace rspl

#pragma once

#include <string>
#include <utility>
#include <vector>

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
  // When non-empty, only these functions are optimized; everything else is
  // still generated but left untouched. Used together with patchAsmFunctions()
  // to update single functions inside an already-emitted .S file.
  std::vector<std::string> patchFunctions;
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

/// Locates a function's text in an ASM listing, as a [start, end) byte range.
/// The function ends at the first following line that starts in column 0.
/// Throws std::runtime_error if the function or its end cannot be found.
std::pair<size_t, size_t> getFunctionStartEnd(const std::string &source,
                                              const std::string &funcName);

/// Replaces each named function in `oldAsm` with its counterpart from
/// `newAsm`, leaving the rest of `oldAsm` byte-for-byte intact.
std::string patchAsmFunctions(const std::string &oldAsm,
                              const std::string &newAsm,
                              const std::vector<std::string> &funcNames);

} // namespace rspl

#include "pipeline.h"

#include "asm_normalize.h"
#include "ast_normalize.h"
#include "asm_writer.h"
#include "ast.h"
#include "ast2asm.h"
#include "preproc.h"
#include "optimizer/asm_optimizer.h"
#include "optimizer/asm_scan_deps.h"
#include "optimizer/eval_cost.h"
#include "state.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sstream>
#include <string>

namespace rspl {

// --- JS parser subprocess -------------------------------------------

static std::string execJsParser(const std::string &rsplPath,
                                bool skipPreproc) {
  const char *scriptPath = std::getenv("RSPL_PARSE_JS");
  std::string cmd;
  if (scriptPath) {
    cmd = std::string("node ") + scriptPath;
  } else {
    cmd = "node scripts/parse.js";
  }
  cmd += skipPreproc ? " --preprocessed " : " ";
  cmd += "\"" + rsplPath + "\"";
  cmd += " 2>&1"; // capture stderr too

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    throw std::runtime_error("Error: cannot start JS parser");
  }
  std::string result;
  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) result += buf;
  int rc = pclose(pipe);
  if (rc != 0) {
    throw std::runtime_error("Error: JS parser exited with code " +
                             std::to_string(rc) + "\n" + result);
  }
  return result;
}

// --- Function patching ----------------------------------------------

std::pair<size_t, size_t> getFunctionStartEnd(const std::string &source,
                                              const std::string &funcName) {
  auto funcIdx = source.find(funcName + ":\n");
  if (funcIdx == std::string::npos) {
    throw std::runtime_error("Function " + funcName +
                             " not found in output file!");
  }
  // The body is indented, so the function ends at the next line starting
  // with an alphanumeric character in column 0.
  for (size_t i = funcIdx; i + 1 < source.size(); ++i) {
    if (source[i] == '\n' &&
        std::isalnum(static_cast<unsigned char>(source[i + 1]))) {
      return {funcIdx, i};
    }
  }
  throw std::runtime_error("Function end not found in output file!");
}

std::string patchAsmFunctions(const std::string &oldAsm,
                              const std::string &newAsm,
                              const std::vector<std::string> &funcNames) {
  std::string out = oldAsm;
  for (const auto &name : funcNames) {
    auto posOld = getFunctionStartEnd(out, name);
    auto posNew = getFunctionStartEnd(newAsm, name);
    out = out.substr(0, posOld.first) +
          newAsm.substr(posNew.first, posNew.second - posNew.first) +
          out.substr(posOld.second);
  }
  return out;
}

// True when this function should take part in optimization. With no patch
// list everything is optimized; otherwise only the listed functions are.
static bool isOptimizeTarget(const TranspileConfig &config,
                             const AsmFunc &fn) {
  if (config.patchFunctions.empty()) return true;
  return std::find(config.patchFunctions.begin(), config.patchFunctions.end(),
                   fn.name) != config.patchFunctions.end();
}

// --- runPipeline (CLI path) -----------------------------------------

TranspileResult runPipeline(const std::string &astJson,
                            const TranspileConfig &config) {
  auto prog = ast::parseJson(astJson);
  astNormalize(prog, config.magma);

  auto functions = ast2asm(prog);

  if (config.optimize) {
    for (auto &fn : functions) {
      if (fn.asm_.empty() || !isOptimizeTarget(config, fn)) continue;
      asmOptimizePattern(fn);
      asmInitDeps(fn);
      evalFunctionCost(fn);
    }
    if (config.reorder) {
      for (auto &fn : functions) {
        if (fn.asm_.empty() || !isOptimizeTarget(config, fn)) continue;
        asmOptimize(fn, config.optimizeTime, config.optWorkers);
      }
      printCumulativeStats();
    } else {
      for (auto &fn : functions) {
        if (fn.asm_.empty() || !isOptimizeTarget(config, fn)) continue;
        fillDelaySlots(fn);
        evalFunctionCost(fn);
      }
    }
  }

  WriteConfig wConfig;
  wConfig.rspqWrapper = config.rspqWrapper;
  wConfig.debugInfo = true;
  wConfig.magma = config.magma;

  auto result = writeASM(prog, functions, wConfig);

  TranspileResult out;
  out.asm_ = result.asm_;
  out.sizeDMEM = result.sizeDMEM;
  out.sizeIMEM = result.sizeIMEM;
  return out;
}

// --- transpileSource (test / library path) --------------------------

TranspileResult transpileSource(const std::string &source,
                                const TranspileConfig &config) {
  // Preprocess in C++ to collect defines (ordered by source appearance)
  std::unordered_map<std::string, DefineEntry> defines;
  std::vector<DefineEntry> defineOrder;
  std::string preprocessed =
      preprocFull(source, defines, config.sourceDir, &defineOrder);

  // Populate source lines from the PREPROCESSED source for debug info.
  // AST line numbers come from the preprocessed text (includes expanded,
  // macros resolved), so the sourceLines must match.
  state.sourceLines.clear();
  std::istringstream srcStream(preprocessed);
  std::string srcLine;
  while (std::getline(srcStream, srcLine)) {
    size_t start = srcLine.find_first_not_of(" \t\r");
    size_t end = srcLine.find_last_not_of(" \t\r");
    if (start != std::string::npos)
      state.sourceLines.push_back(srcLine.substr(start, end - start + 1));
    else
      state.sourceLines.push_back("");
  }

  // Write preprocessed source to temp file
  std::string tmpPath = "/tmp/rspl_test_source.rspl";
  {
    std::ofstream f(tmpPath);
    if (!f) throw std::runtime_error("Cannot write temp file");
    f << preprocessed;
  }

  // Call JS parser (skip its own preprocessor since we already did it)
  std::string astJson = execJsParser(tmpPath, true);

  // Parse AST
  auto prog = ast::parseJson(astJson);

  // Transfer collected defines to the program in source order.
  // Filter out defines that were later #undef'd (still in the map).
  for (const auto &def : defineOrder) {
    if (defines.count(def.name))
      prog.defines.push_back({def.name, def.value});
  }

  astNormalize(prog, config.magma);

  // Generate ASM
  auto functions = ast2asm(prog);

  // Match JS pipeline: writeASM runs before patterns to advance state.line
  // so that optimizer-generated instructions (e.g. branchJump's ori $ra)
  // pick up ASM output line numbers instead of stale source line numbers.
  if (config.optimize || config.debugInfo) {
    WriteConfig wCfg;
    wCfg.rspqWrapper = config.rspqWrapper;
    wCfg.debugInfo = config.debugInfo;
    wCfg.magma = config.magma;
    writeASM(prog, functions, wCfg);
  }

  if (config.optimize) {
    for (auto &fn : functions) {
      if (fn.asm_.empty() || !isOptimizeTarget(config, fn)) continue;
      asmOptimizePattern(fn);
      asmInitDeps(fn);
      evalFunctionCost(fn);
    }
    if (config.reorder) {
      for (auto &fn : functions) {
        if (fn.asm_.empty() || !isOptimizeTarget(config, fn)) continue;
        asmOptimize(fn, config.optimizeTime, config.optWorkers);
      }
      printCumulativeStats();
    } else {
      for (auto &fn : functions) {
        if (fn.asm_.empty() || !isOptimizeTarget(config, fn)) continue;
        fillDelaySlots(fn);
        evalFunctionCost(fn);
      }
    }
  } else if (config.debugInfo) {
    // When debugInfo is on but optimize is off, still run pattern
    // optimizations and cycle evaluation so the debug output contains
    // meaningful cycle counts.
    for (auto &fn : functions) {
      if (fn.asm_.empty() || !isOptimizeTarget(config, fn)) continue;
      asmOptimizePattern(fn);
      asmInitDeps(fn);
      evalFunctionCost(fn);
    }
  }

  TranspileResult result;

  WriteConfig wConfig;
  wConfig.rspqWrapper = config.rspqWrapper;
  wConfig.debugInfo = config.debugInfo;
  wConfig.magma = config.magma;

  auto writeResult = writeASM(prog, functions, wConfig);
  result.asm_ = writeResult.asm_;
  result.sizeDMEM = writeResult.sizeDMEM;
  result.sizeIMEM = writeResult.sizeIMEM;
  while (!result.asm_.empty() && result.asm_.back() == '\n')
    result.asm_.pop_back();

  return result;
}

} // namespace rspl

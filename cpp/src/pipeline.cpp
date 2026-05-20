#include "pipeline.h"

#include "asm_normalize.h"
#include "asm_writer.h"
#include "ast.h"
#include "ast2asm.h"
#include "preproc.h"
#include "optimizer/asm_optimizer.h"
#include "optimizer/asm_scan_deps.h"
#include "optimizer/eval_cost.h"
#include "state.h"

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

// --- runPipeline (CLI path) -----------------------------------------

int runPipeline(const std::string &astJson, bool rspqWrapper,
                bool optimize) {
  auto prog = ast::parseJson(astJson);

  auto functions = ast2asm(prog);

  if (optimize) {
    for (auto &fn : functions) {
      if (fn.asm_.empty()) continue;
      asmOptimizePattern(fn);
      asmInitDeps(fn);
      fillDelaySlots(fn);
      evalFunctionCost(fn);
    }
  }

  if (!rspqWrapper) {
    std::cout << "## Raw RSP assembly (no wrapper)\n";
    for (const auto &fn : functions) {
      if (fn.asm_.empty()) continue;
      std::cout << fn.name << ":\n";
      for (const auto &inst : fn.asm_) {
        if (inst.type == AsmType::LABEL) {
          std::cout << inst.label << ":\n";
        } else {
          std::cout << "  " << inst.op;
          for (size_t i = 0; i < inst.args.size(); ++i) {
            std::cout << (i == 0 ? " " : ", ") << inst.args[i];
          }
          std::cout << "\n";
        }
      }
      std::cout << "\n";
    }
    return 0;
  }

  WriteConfig config;
  config.rspqWrapper = true;
  config.debugInfo = true;

  auto result = writeASM(prog, functions, config);

  std::cout << result.asm_ << std::flush;

  std::cerr << "// DMEM: " << result.sizeDMEM
            << " bytes, IMEM: " << result.sizeIMEM << " bytes\n";

  return 0;
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

  // Generate ASM
  auto functions = ast2asm(prog);

  // Match JS pipeline: writeASM runs before patterns to advance state.line
  // so that optimizer-generated instructions (e.g. branchJump's ori $ra)
  // pick up ASM output line numbers instead of stale source line numbers.
  if (config.optimize || config.debugInfo) {
    WriteConfig wCfg;
    wCfg.rspqWrapper = config.rspqWrapper;
    wCfg.debugInfo = config.debugInfo;
    writeASM(prog, functions, wCfg);
  }

  if (config.optimize) {
    for (auto &fn : functions) {
      if (fn.asm_.empty()) continue;
      asmOptimizePattern(fn);
      asmInitDeps(fn);
      fillDelaySlots(fn);
      evalFunctionCost(fn);
    }
  } else if (config.debugInfo) {
    // When debugInfo is on but optimize is off, still run pattern
    // optimizations and cycle evaluation so the debug output contains
    // meaningful cycle counts.
    for (auto &fn : functions) {
      if (fn.asm_.empty()) continue;
      asmOptimizePattern(fn);
      asmInitDeps(fn);
      evalFunctionCost(fn);
    }
  }

  TranspileResult result;

  WriteConfig wConfig;
  wConfig.rspqWrapper = config.rspqWrapper;
  wConfig.debugInfo = config.debugInfo;

  auto writeResult = writeASM(prog, functions, wConfig);
  result.asm_ = writeResult.asm_;
  while (!result.asm_.empty() && result.asm_.back() == '\n')
    result.asm_.pop_back();

  return result;
}

} // namespace rspl

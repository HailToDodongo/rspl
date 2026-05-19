#include "pipeline.h"

#include "asm_normalize.h"
#include "asm_writer.h"
#include "ast.h"
#include "ast2asm.h"
#include "preproc.h"
#include "optimizer/asm_optimizer.h"
#include "optimizer/asm_scan_deps.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
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
  // Preprocess in C++ to collect defines
  std::unordered_map<std::string, DefineEntry> defines;
  std::string preprocessed = preprocFull(source, defines, config.sourceDir);

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

  // Transfer collected defines to the program
  for (const auto &[name, def] : defines) {
    prog.defines.push_back({def.name, def.value});
  }

  // Generate ASM
  auto functions = ast2asm(prog);

  if (config.optimize) {
    for (auto &fn : functions) {
      if (fn.asm_.empty()) continue;
      asmOptimizePattern(fn);
      asmInitDeps(fn);
      fillDelaySlots(fn);
    }
  }

  TranspileResult result;

  if (!config.rspqWrapper) {
    std::ostringstream out;
    for (size_t fi = 0; fi < functions.size(); ++fi) {
      const auto &fn = functions[fi];
      if (fn.asm_.empty()) continue;

      // Emit .align from @Align(N) annotation
      for (const auto &ann : fn.annotations) {
        if (ann.name == "Align" && !ann.value.empty()) {
          int alignBytes = std::stoi(ann.value);
          int alignExp = static_cast<int>(std::log2(alignBytes));
          if (alignExp > 0 && (1 << alignExp) == alignBytes) {
            out << ".align " << alignExp << "\n";
          }
        }
      }

      out << fn.name << ":\n";
      for (const auto &inst : fn.asm_) {
        if (inst.type == AsmType::LABEL) {
          out << "  " << inst.label << ":\n";
        } else {
          out << "  " << inst.op;
          for (size_t i = 0; i < inst.args.size(); ++i) {
            out << (i == 0 ? " " : ", ") << inst.args[i];
          }
          out << "\n";
        }
      }
    }
    result.asm_ = out.str();
    while (!result.asm_.empty() && result.asm_.back() == '\n')
      result.asm_.pop_back();
    return result;
  }

  WriteConfig wConfig;
  wConfig.rspqWrapper = true;
  wConfig.debugInfo = config.debugInfo;

  auto writeResult = writeASM(prog, functions, wConfig);
  result.asm_ = writeResult.asm_;
  while (!result.asm_.empty() && result.asm_.back() == '\n')
    result.asm_.pop_back();

  return result;
}

} // namespace rspl

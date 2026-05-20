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
  // Populate source lines for debug info
  state.sourceLines.clear();
  std::istringstream srcStream(source);
  std::string srcLine;
  while (std::getline(srcStream, srcLine)) {
    // Trim leading/trailing whitespace like JS
    size_t start = srcLine.find_first_not_of(" \t\r");
    size_t end = srcLine.find_last_not_of(" \t\r");
    if (start != std::string::npos)
      state.sourceLines.push_back(srcLine.substr(start, end - start + 1));
    else
      state.sourceLines.push_back("");
  }

  // Preprocess in C++ to collect defines (ordered by source appearance)
  std::unordered_map<std::string, DefineEntry> defines;
  std::vector<DefineEntry> defineOrder;
  std::string preprocessed =
      preprocFull(source, defines, config.sourceDir, &defineOrder);

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

      // Initialize lastCycle from first instruction (matching JS behavior)
      int lastCycle = fn.asm_.empty() ? 0 : fn.asm_[0].debug.cycle;
      for (const auto &inst : fn.asm_) {
        if (inst.type == AsmType::LABEL) {
          // Check for @Tag annotation on this label
          std::string tag;
          for (const auto &ann : inst.annotations) {
            if (ann.name == "Tag")
              tag = "TAG_" + ann.value + ": ";
          }
          out << "  " << tag << inst.label << ":\n";
        } else {
          // Build the raw instruction string (without prefix or tag)
          // matching JS stringifyInstr: op + " " + args.join(", ")
          std::ostringstream rawIss;
          rawIss << inst.op;
          for (size_t i = 0; i < inst.args.size(); ++i) {
            rawIss << (i == 0 ? " " : ", ") << inst.args[i];
          }
          std::string rawInstr = rawIss.str();

          // Determine tag prefix
          std::string tag;
          for (const auto &ann : inst.annotations) {
            if (ann.name == "Tag")
              tag = "TAG_" + ann.value + ": ";
          }

          // Build the full line
          std::string instr;
          if (config.debugInfo) {
            // Pad instruction to 51 chars, then prepend prefix + tag
            // (matching JS: padEnd(50) produces 51-char instruction field)
            std::string padded = rawInstr;
            if (padded.size() < 51)
              padded.append(51 - padded.size(), ' ');
            instr = "  " + tag + padded;

            // Build debug info string
            std::ostringstream di;
            if (inst.debug.lineRSPL) {
              // Cycle string
              std::string cycleStr = "     ^";
              int cycleDiff = inst.debug.cycle - lastCycle;
              if (cycleDiff != 0) {
                std::string stars;
                if (cycleDiff > 1)
                  stars.append(cycleDiff - 1, '*');
                cycleStr = stars + std::to_string(inst.debug.cycle);
                // Pad to 6 chars
                if (cycleStr.size() < 6)
                  cycleStr.insert(0, 6 - cycleStr.size(), ' ');
              }
              std::string lineStr = std::to_string(inst.debug.lineRSPL);
              if (lineStr.size() < 4)
                lineStr.append(4 - lineStr.size(), ' ');
              di << "## L:" << lineStr << " | " << cycleStr << " | ";
              if (inst.debug.lineRSPL > 0 &&
                  inst.debug.lineRSPL <=
                      static_cast<int>(state.sourceLines.size())) {
                di << state.sourceLines[inst.debug.lineRSPL - 1];
              }
            }

            // Function args
            if (!inst.funcArgs.empty()) {
              di << " ## Args: ";
              for (size_t i = 0; i < inst.funcArgs.size(); ++i) {
                if (i) di << ", ";
                di << inst.funcArgs[i];
              }
            }

            // Barrier mask
            if (inst.barrierMask) {
              std::ostringstream bs;
              bs << std::hex << std::uppercase << inst.barrierMask;
              di << " ## Barrier: 0x" << bs.str();
            }

            instr += di.str();
          } else {
            instr = "  " + tag + rawInstr;
          }

          out << instr << "\n";
          lastCycle = inst.debug.cycle;
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

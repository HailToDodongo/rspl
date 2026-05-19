#pragma once

#include "asm.h"
#include "ast.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

struct WriteConfig {
  bool rspqWrapper = true;
  bool debugInfo = true;
};

struct AsmWriteResult {
  std::string asm_;
  int sizeDMEM = 0;
  int sizeIMEM = 0;
  // lineMap: RSPL source line -> vector of ASM lines
  std::unordered_map<int, std::vector<int>> lineMap;
  // lineDepMap: ASM line -> [min, max] reorder range
  std::unordered_map<int, std::pair<int, int>> lineDepMap;
  // lineOptMap: original ASM line -> optimized ASM line
  std::unordered_map<int, int> lineOptMap;
  // lineCycleMap: optimized ASM line -> cycle
  std::unordered_map<int, int> lineCycleMap;
  // lineStallMap: optimized ASM line -> stall count
  std::unordered_map<int, int> lineStallMap;
};

AsmWriteResult writeASM(const ast::Program &ast,
                        const std::vector<AsmFunc> &functions,
                        const WriteConfig &config);

// Extracted for testing: format a single instruction to text
std::string stringifyInstr(const AsmInst &inst);

} // namespace rspl

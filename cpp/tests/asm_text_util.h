#pragma once
// Shared test helper: parse text with bracket annotations like "[0] nop",
// "[^] vadd ..." into AsmInst vectors. Lines with "# unlikely" clear the
// likely-branch flags (trace segments record executed, not-taken branches).
#include "asm.h"

#include <sstream>
#include <string>
#include <vector>

inline std::vector<rspl::AsmInst> textToAsmLines(const std::string &text) {
  using namespace rspl;
  std::vector<AsmInst> lines;
  std::istringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    // strip leading bracket annotation [*]
    auto rb = line.find(']');
    if (rb == std::string::npos) continue;
    line = line.substr(rb + 1);

    // trim
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    size_t end = line.find_last_not_of(" \t");
    line = line.substr(start, end - start + 1);
    if (line.empty()) continue;

    // remove trailing comment
    auto hashPos = line.find('#');
    bool unlikely = false;
    if (hashPos != std::string::npos) {
      if (line.find("unlikely", hashPos) != std::string::npos) unlikely = true;
      line = line.substr(0, hashPos);
      // trim again
      end = line.find_last_not_of(" \t");
      if (end == std::string::npos) continue;
      line = line.substr(0, end + 1);
    }

    std::istringstream ls(line);
    std::string op;
    ls >> op;
    std::vector<std::string> args;
    std::string arg;
    while (ls >> arg) {
      if (arg.back() == ',') arg.pop_back();
      args.push_back(arg);
    }
    AsmInst inst;
    if (op == "nop")
      inst = asmNOP();
    else
      inst = asmOp(op, args);

    if (inst.opFlags & OpFlag::OP_FLAG_IS_BRANCH) {
      if (unlikely) {
        inst.opFlags &=
            ~(OpFlag::OP_FLAG_LIKELY_BRANCH | OpFlag::OP_FLAG_IS_LIKELY);
      } else {
        inst.opFlags |=
            (OpFlag::OP_FLAG_LIKELY_BRANCH | OpFlag::OP_FLAG_IS_LIKELY);
      }
    }
    lines.push_back(std::move(inst));
  }
  return lines;
}

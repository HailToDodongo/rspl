#include "asm_writer.h"
#include "asm.h"
#include "ast.h"
#include "registers.h"
#include "state.h"
#include "types.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace rspl {

std::string stringifyInstr(const AsmInst &inst) {
  if (inst.op.empty()) return inst.label + ":";
  if (inst.args.empty()) return inst.op;
  std::ostringstream ss;
  ss << inst.op;
  for (size_t i = 0; i < inst.args.size(); ++i) {
    ss << (i == 0 ? " " : ", ") << inst.args[i];
  }
  return ss.str();
}

static std::string makePadding(size_t len, size_t target) {
  if (len >= target) return " ";
  return std::string(target - len, ' ');
}

AsmWriteResult writeASM(const ast::Program &ast,
                        const std::vector<AsmFunc> &functions,
                        const WriteConfig &config) {
  state.func = "(ASM)";
  state.line = 0;

  AsmWriteResult res;
  std::ostringstream out;
  int asmLine = 0; // physical ASM line count

  auto writeLine = [&](const std::string &line) {
    out << line << "\n";
    ++asmLine;
    ++state.line;
  };

  auto writeLines = [&](const std::vector<std::string> &lines) {
    for (const auto &l : lines) {
      writeLine(l);
    }
  };

  writeLine("## Auto-generated file, transpiled with RSPL");

  // Defines from preprocessor
  for (const auto &def : ast.defines) {
    writeLine("#define " + def.name + " " + def.value);
  }

  // Includes
  for (const auto &inc : ast.includes) {
    std::string path = inc;
    // Strip surrounding quotes if present
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
      path = path.substr(1, path.size() - 2);
    }
    bool local = !path.empty() && path[0] == '.';
    writeLine(std::string("#include ") + (local ? "\"" : "<") + path +
              (local ? "\"" : ">"));
  }

  writeLines({
      "", ".set noreorder", ".set noat", ".set nomacro", "",
  });

  // Undefines + equ for scalar registers
  for (const auto &reg : reg::REGS_SCALAR) {
    writeLine("#undef " + reg.substr(1));
  }
  for (size_t i = 0; i < reg::REGS_SCALAR.size(); ++i) {
    writeLine(".equ hex." + reg::REGS_SCALAR[i] + ", " +
              std::to_string(i));
  }
  writeLines({"#define vco 0", "#define vcc 1", "#define vce 2"});

  writeLines({"", ".data", "  RSPQ_BeginOverlayHeader"});

  // Command list
  int maxResultType = -1;
  for (const auto &fn : functions) {
    if (fn.type == "command") {
      int rt = fn.resultType.value_or(-1);
      if (rt > maxResultType) maxResultType = rt;
    }
  }
  maxResultType = std::max(maxResultType, -1);

  std::vector<std::string> commandList(maxResultType + 1,
                                       "    RSPQ_DefineCommand RSPQ_Loop, 4");
  for (const auto &fn : functions) {
    if (fn.type == "command" && fn.resultType.has_value()) {
      std::string name =
          fn.nameOverride.empty() ? fn.name : fn.nameOverride;
      commandList[fn.resultType.value()] =
          "    RSPQ_DefineCommand " + name + ", " +
          std::to_string(std::max(fn.argSize * 4, 4));
    }
  }
  writeLines(commandList);
  writeLines({"  RSPQ_EndOverlayHeader", ""});

  // State sections
  // Collect state vars from AST
  std::vector<ast::StateVarDef> stateVars;
  std::vector<ast::StateVarDef> dataVars;
  std::vector<ast::StateVarDef> bssVars;

  for (const auto &sec : ast.states) {
    for (const auto &v : sec.vars) {
      if (sec.name == "state" || sec.name.empty())
        stateVars.push_back(v);
      else if (sec.name == "data")
        dataVars.push_back(v);
      else if (sec.name == "bss" || sec.name == "temp_state")
        bssVars.push_back(v);
      else
        stateVars.push_back(v); // default to state
    }
  }

  int totalSaveByteSize = 0;
  int totalTextSize = 0;

  bool hasState = std::any_of(stateVars.begin(), stateVars.end(),
                              [](auto &v) { return !v.isExtern; });

  if (hasState) {
    writeLine("  RSPQ_BeginSavedState");
    writeLine("    STATE_MEM_START:");

    for (const auto &sv : stateVars) {
      if (sv.isExtern) continue;
      int arraySize = 1;
      for (auto dim : sv.arraySize) arraySize *= dim;
      if (arraySize < 1) arraySize = 1;
      int byteSize = (TYPE_SIZE.count(sv.varType)
                          ? TYPE_SIZE.at(sv.varType)
                          : 4) *
                     arraySize;

      int align = TYPE_ALIGNMENT.count(sv.varType)
                      ? TYPE_ALIGNMENT.at(sv.varType)
                      : 0;
      if (sv.align != 0) {
        align = static_cast<int>(std::log2(sv.align));
      }
      if (align > 0) {
        writeLine("    .align " + std::to_string(align));
      }

      if (sv.value.empty()) {
        writeLine("    " + sv.varName + ": .ds.b " +
                  std::to_string(byteSize));
      } else {
        auto asmDefIt = TYPE_ASM_DEF.find(sv.varType);
        std::string asmType = (asmDefIt != TYPE_ASM_DEF.end())
                                  ? asmDefIt->second.type
                                  : "word";
        int asmCount = (asmDefIt != TYPE_ASM_DEF.end())
                           ? asmDefIt->second.count
                           : 1;
        int arrayCount = arraySize / asmCount;
        if (arrayCount < 1) arrayCount = 1;
        // Write data with correct type
        int totalCount = asmCount * arrayCount;
        std::vector<double> data(totalCount, 0.0);
        for (size_t i = 0; i < sv.value.size() && i < static_cast<size_t>(totalCount); ++i) {
          data[i] = sv.value[i];
        }
        std::ostringstream ss;
        for (int i = 0; i < totalCount; ++i) {
          if (i) ss << ", ";
          ss << static_cast<int64_t>(data[i]);
        }
        writeLine("    " + sv.varName + ": ." + asmType + " " + ss.str());
      }
      totalSaveByteSize += byteSize;
    }

    writeLine("    STATE_MEM_END:");
    writeLine("  RSPQ_EndSavedState");
  } else {
    writeLine("  RSPQ_EmptySavedState");
  }

  // Helper to emit a single state var (with alignment and size)
  auto emitStateVar = [&](const ast::StateVarDef &sv) {
    int arraySize = 1;
    for (auto dim : sv.arraySize) arraySize *= dim;
    if (arraySize < 1) arraySize = 1;
    int byteSize = (TYPE_SIZE.count(sv.varType)
                        ? TYPE_SIZE.at(sv.varType)
                        : 4) *
                   arraySize;
    int align = TYPE_ALIGNMENT.count(sv.varType)
                    ? TYPE_ALIGNMENT.at(sv.varType)
                    : 0;
    if (sv.align != 0)
      align = static_cast<int>(std::log2(sv.align));
    if (align > 0)
      writeLine("    .align " + std::to_string(align));
    writeLine("    " + sv.varName + ": .ds.b " +
              std::to_string(byteSize));
  };

  // Data section
  if (!dataVars.empty()) {
    writeLine("");
    for (const auto &dv : dataVars) {
      if (dv.isExtern) continue;
      emitStateVar(dv);
    }
  }

  // BSS section
  if (!bssVars.empty()) {
    writeLine("");
    writeLine(".bss");
    writeLine("  TEMP_STATE_MEM_START:");
    for (const auto &bv : bssVars) {
      if (bv.isExtern) continue;
      emitStateVar(bv);
    }
    writeLine("  TEMP_STATE_MEM_END:");
  }

  writeLines({"", ".text", "OVERLAY_CODE_START:", ""});

  // Function bodies
  for (const auto &fn : functions) {
    if (fn.asm_.empty()) continue;

    // Emit .align from @Align(N) annotation
    for (const auto &ann : fn.annotations) {
      if (ann.name == "Align" && !ann.value.empty()) {
        int alignBytes = std::stoi(ann.value);
        int alignExp = static_cast<int>(std::log2(alignBytes));
        if (alignExp > 0 && (1 << alignExp) == alignBytes) {
          writeLine(".align " + std::to_string(alignExp));
        }
      }
    }

    writeLine(fn.name + ":");

    AsmInst lastInst;
    bool hasLast = false;

    for (const auto &inst : fn.asm_) {
      // Assign line numbers for debug info
      // (we don't have mutable AsmInst in this const ref,
      //  so skip the line number tracking for now)

      if (inst.type == AsmType::LABEL) {
        writeLine("  " + inst.label + ":");
      } else {
        std::string instr = "  " + stringifyInstr(inst);
        // Pad to 50 chars for alignment
        if (config.debugInfo) {
          instr += makePadding(instr.size(), 50);
        }
        writeLine(instr);
        totalTextSize += 4; // each MIPS instruction = 4 bytes
      }

      if (inst.type == AsmType::OP) {
        lastInst = inst;
        hasLast = true;
      }
    }
  }

  writeLine("");

  if (!config.rspqWrapper) return res;

  writeLine("OVERLAY_CODE_END:");
  writeLine("");

  // Register defines
  for (size_t i = 0; i < reg::REGS_SCALAR.size(); ++i) {
    if (i == 1) continue; // skip $at
    writeLine("#define " + reg::REGS_SCALAR[i].substr(1) + " $" +
              std::to_string(i));
  }

  writeLines({"", ".set at", ".set macro"});

  // Post includes
  for (const auto &inc : ast.postIncludes) {
    std::string path = inc;
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
      path = path.substr(1, path.size() - 2);
    }
    bool local = !path.empty() && path[0] == '.';
    writeLine(std::string("#include ") + (local ? "\"" : "<") + path +
              (local ? "\"" : ">"));
  }

  res.asm_ = out.str();
  res.sizeDMEM = totalSaveByteSize;
  res.sizeIMEM = totalTextSize;
  return res;
}

} // namespace rspl

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

// --- Magma vertex-attribute annotations -------------------------------

static std::string getAnnoValue(const AsmInst &inst, const char *name) {
  for (const auto &ann : inst.cold->annotations) {
    if (ann.name == name) return ann.value;
  }
  return {};
}

// "@AttrPatch(name:op)" — only the first two parts are used
static std::string getAttrPatchName(const AsmInst &inst) {
  std::string val = getAnnoValue(inst, "AttrPatch");
  if (val.empty()) return {};
  auto colon = val.find(':');
  return (colon == std::string::npos) ? val : val.substr(0, colon);
}

static std::string getAttrPatchOp(const AsmInst &inst) {
  std::string val = getAnnoValue(inst, "AttrPatch");
  auto colon = val.find(':');
  if (colon == std::string::npos) return {};
  std::string rest = val.substr(colon + 1);
  auto next = rest.find(':');
  return (next == std::string::npos) ? rest : rest.substr(0, next);
}

static std::string getAttrLoaderLabel(const AsmInst &inst) {
  std::string val = getAnnoValue(inst, "AttrLoader");
  if (val.empty()) return {};
  return "LOAD_" + val + std::to_string(inst.debug.lineRSPL);
}

static std::string getAttrPatchLabel(const AsmInst &inst) {
  std::string name = getAttrPatchName(inst);
  if (name.empty()) return {};
  return "PATCH_" + name + std::to_string(inst.debug.lineRSPL);
}

static std::string getAsmLabels(const AsmInst &inst) {
  std::string labels;
  for (const auto &label : {getAttrLoaderLabel(inst), getAttrPatchLabel(inst)}) {
    if (!label.empty()) labels += label + ": ";
  }
  return labels;
}

std::string stringifyInstr(const AsmInst &inst) {
  if (inst.op == 0) return inst.cold->label + ":";
  std::string prefix = getAsmLabels(inst);
  if (inst.args.empty()) return prefix + getOpcodeName(inst.op);
  std::ostringstream ss;
  ss << prefix << getOpcodeName(inst.op);
  for (size_t i = 0; i < inst.args.size(); ++i) {
    ss << (i == 0 ? " " : ", ") << inst.args[i];
  }
  return ss.str();
}

static std::string makePadding(size_t len, size_t target) {
  if (len >= target) return " ";
  return std::string(target - len, ' ');
}

// --- Magma header ------------------------------------------------------

namespace {

struct AttrEntry {
  std::vector<const AsmInst *> loaders;
  std::vector<const AsmInst *> patches;
};

// Group instructions by the vertex attribute they load or patch.
std::unordered_map<std::string, AttrEntry>
collectAttrLoaders(const std::vector<AsmFunc> &functions) {
  std::unordered_map<std::string, AttrEntry> res;
  for (const auto &fn : functions) {
    for (const auto &inst : fn.asm_) {
      std::string loader = getAnnoValue(inst, "AttrLoader");
      if (!loader.empty()) res[loader].loaders.push_back(&inst);

      std::string patch = getAttrPatchName(inst);
      if (!patch.empty()) res[patch].patches.push_back(&inst);
    }
  }
  return res;
}

// Emits one state variable, returning its size in bytes.
// Mirrors the JS writeStateVar().
int writeStateVar(const ast::StateVarDef &sv,
                  const std::function<void(const std::string &)> &writeLine) {
  int64_t arraySize = 1;
  for (auto dim : sv.arraySize) arraySize *= dim;
  if (arraySize < 1) arraySize = 1;

  int typeSize = TYPE_SIZE.count(sv.varType) ? TYPE_SIZE.at(sv.varType) : 4;
  int byteSize = static_cast<int>(typeSize * arraySize);

  int align =
      TYPE_ALIGNMENT.count(sv.varType) ? TYPE_ALIGNMENT.at(sv.varType) : 0;
  if (sv.align != 0) {
    double alignPow = std::log2(static_cast<double>(sv.align));
    if (alignPow != std::floor(alignPow)) {
      state.throwError("Invalid align value '" + std::to_string(sv.align) +
                       "', must be a power of 2");
    }
    align = static_cast<int>(alignPow);
  }
  if (align > 0) writeLine("    .align " + std::to_string(align));

  if (sv.value.empty()) {
    writeLine("    " + sv.varName + ": .ds.b " + std::to_string(byteSize));
    return byteSize;
  }

  auto asmDefIt = TYPE_ASM_DEF.find(sv.varType);
  std::string asmType =
      (asmDefIt != TYPE_ASM_DEF.end()) ? asmDefIt->second.type : "word";
  int asmCount = (asmDefIt != TYPE_ASM_DEF.end()) ? asmDefIt->second.count : 1;

  size_t total = static_cast<size_t>(asmCount * arraySize);
  if (sv.value.size() > total) {
    state.throwError("Too many initializers for '" + sv.varName + "' (" +
                     std::to_string(sv.value.size()) + " > " +
                     std::to_string(total) + ")");
  }
  std::vector<int64_t> data(total, 0);
  for (size_t i = 0; i < sv.value.size(); ++i) data[i] = sv.value[i];

  std::ostringstream ss;
  for (size_t i = 0; i < data.size(); ++i) {
    if (i) ss << ", ";
    ss << data[i];
  }
  writeLine("    " + sv.varName + ": ." + asmType + " " + ss.str());
  return byteSize;
}

int writeMagmaHeader(const ast::Program &ast,
                     const std::vector<AsmFunc> &functions,
                     const std::function<void(const std::string &)> &writeLine,
                     const std::function<void(const std::vector<std::string> &)> &writeLines) {
  int totalSaveByteSize = 0;

  writeLines({"", "MgBeginShaderUniforms"});
  for (const auto &uniform : ast.uniforms) {
    writeLine("  MgBeginUniform " + uniform.name + ", " +
              std::to_string(uniform.binding.value_or(0)));
    for (const auto &sv : uniform.state) {
      if (sv.isExtern) continue;
      totalSaveByteSize += writeStateVar(sv, writeLine);
    }
    writeLines({"  MgEndUniform", ""});
  }
  writeLines({"MgEndShaderUniforms", ""});

  auto attrLoaders = collectAttrLoaders(functions);

  writeLine("MgBeginVertexInput");
  for (const auto &attr : ast.attributes) {
    writeLine("  MgBeginVertexAttribute " +
              std::to_string(attr.binding.value_or(0)) + ", " +
              (attr.optional ? "1" : "0"));

    auto it = attrLoaders.find(attr.name);
    if (it != attrLoaders.end()) {
      if (!it->second.loaders.empty()) {
        std::string line = "    MgVertexAttributeLoaders ";
        for (size_t i = 0; i < it->second.loaders.size(); ++i) {
          if (i) line += ", ";
          line += getAttrLoaderLabel(*it->second.loaders[i]);
        }
        writeLine(line);
      }
      for (const auto *patch : it->second.patches) {
        writeLine("    MgBeginVertexAttributePatch " +
                  getAttrPatchLabel(*patch));
        std::string op = getAttrPatchOp(*patch);
        writeLine("      " + (op.empty() ? std::string("nop") : op));
        writeLine("    MgEndVertexAttributePatch");
      }
    }
    writeLines({"  MgEndVertexAttribute", ""});
  }
  writeLines({"MgEndVertexInput", ""});

  writeLine("MgBeginShader");
  return totalSaveByteSize;
}

} // namespace

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
    writeLine(def.value.empty() ? "#define " + def.name
                                : "#define " + def.name + " " + def.value);
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

  int totalSaveByteSize = 0;
  int totalTextSize = 0;

  if (config.magma) {
    totalSaveByteSize += writeMagmaHeader(ast, functions, writeLine, writeLines);
  } else {
  writeLines({"", ".data", "  RSPQ_BeginOverlayHeader"});

  // Command list
  int maxResultType = -1;
  for (const auto &fn : functions) {
    if (fn.type == FuncType::Command) {
      int rt = fn.resultType.value_or(-1);
      if (rt > maxResultType) maxResultType = rt;
    }
  }
  maxResultType = std::max(maxResultType, -1);

  std::vector<std::string> commandList(maxResultType + 1,
                                       "    RSPQ_DefineCommand RSPQ_Loop, 4");
  for (const auto &fn : functions) {
    if (fn.type == FuncType::Command && fn.resultType.has_value()) {
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

  bool hasState = std::any_of(stateVars.begin(), stateVars.end(),
                              [](auto &v) { return !v.isExtern; });

  if (hasState) {
    writeLine("  RSPQ_BeginSavedState");
    writeLine("    STATE_MEM_START:");

    for (const auto &sv : stateVars) {
      if (sv.isExtern) continue;
      totalSaveByteSize += writeStateVar(sv, writeLine);
    }

    writeLine("    STATE_MEM_END:");
    writeLine("  RSPQ_EndSavedState");
  } else {
    writeLine("  RSPQ_EmptySavedState");
  }

  // Data section — same emitter as the saved state, values included
  if (!dataVars.empty()) {
    writeLine("");
    for (const auto &dv : dataVars) {
      if (dv.isExtern) continue;
      totalSaveByteSize += writeStateVar(dv, writeLine);
    }
  }

  // BSS section
  if (!bssVars.empty()) {
    writeLine("");
    writeLine(".bss");
    writeLine("  TEMP_STATE_MEM_START:");
    for (const auto &bv : bssVars) {
      if (bv.isExtern) continue;
      totalSaveByteSize += writeStateVar(bv, writeLine);
    }
    writeLine("  TEMP_STATE_MEM_END:");
  }

  writeLines({"", ".text", "OVERLAY_CODE_START:", ""});
  }

  // For non-wrapper output, reset here — the headers above were only
  // needed for state.line to advance correctly (matching JS asmWriter.js:184-186)
  if (!config.rspqWrapper) {
    state.line = 1;
    out.str("");
    out.clear();
  }

  // Function bodies
  auto writeFunction = [&](const AsmFunc &fn) {
    if (fn.asm_.empty()) return;

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

    // the shader entry point is the start of the shader block itself
    if (fn.type != FuncType::Shader) writeLine(fn.name + ":");

    // Track last cycle for debug info (matching JS asmWriter.js)
    int lastCycle = fn.asm_.empty() ? 0 : fn.asm_[0].debug.cycle;

    for (const auto &inst : fn.asm_) {
      if (inst.type == AsmType::LABEL) {
        std::string tag;
        for (const auto &ann : inst.cold->annotations) {
          if (ann.name == "Tag")
            tag = "TAG_" + ann.value + ": ";
        }
        writeLine("  " + tag + inst.cold->label + ":");
      } else {
        // Build raw instruction string (matching JS stringifyInstr)
        std::string rawInstr = stringifyInstr(inst);

        // Determine tag prefix
        std::string tag;
        for (const auto &ann : inst.cold->annotations) {
          if (ann.name == "Tag")
            tag = "TAG_" + ann.value + ": ";
        }

        std::string instr;
        if (config.debugInfo) {
          // Pad instruction to 51 chars, then prepend prefix + tag
          std::string padded = rawInstr;
          if (padded.size() < 51)
            padded.append(51 - padded.size(), ' ');
          instr = "  " + tag + padded;

          // Build debug info string
          std::ostringstream di;
          if (inst.debug.lineRSPL) {
            std::string cycleStr = "     ^";
            int cycleDiff = inst.debug.cycle - lastCycle;
            if (cycleDiff != 0) {
              std::string stars;
              if (cycleDiff > 1)
                stars.append(cycleDiff - 1, '*');
              cycleStr = stars + std::to_string(inst.debug.cycle);
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

          if (!inst.cold->funcArgs.empty()) {
            di << " ## Args: ";
            for (size_t i = 0; i < inst.cold->funcArgs.size(); ++i) {
              if (i) di << ", ";
              di << inst.cold->funcArgs[i];
            }
          }

          if (inst.barrierMask) {
            std::ostringstream bs;
            bs << std::hex << std::uppercase << inst.barrierMask;
            di << " ## Barrier: 0x" << bs.str();
          }

          instr += di.str();
        } else {
          instr = "  " + tag + rawInstr;
        }

        writeLine(instr);
        // JS: only real ops count toward text size / cycle tracking,
        // inline ASM (.set directives, macros) is excluded
        if (inst.type == AsmType::OP) {
          totalTextSize += 4;
          lastCycle = inst.debug.cycle;
        }
      }
    }
  };

  // The shader body directly follows MgBeginShader, so it goes first.
  if (config.magma) {
    for (const auto &fn : functions) {
      if (fn.type == FuncType::Shader) writeFunction(fn);
    }
  }
  for (const auto &fn : functions) {
    if (fn.type == FuncType::Function || fn.type == FuncType::Command) {
      writeFunction(fn);
    }
  }

  writeLine("");

  if (!config.rspqWrapper) {
    res.asm_ = out.str();
    return res;
  }

  writeLine(config.magma ? "MgEndShader" : "OVERLAY_CODE_END:");
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

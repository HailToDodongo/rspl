#include "state.h"
#include "registers.h"
#include "types.h"

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <stdexcept>

namespace rspl {

// --- State constructor / reset ----------------------------------------

static const std::vector<std::string> LABELS = {
    "RSPQ_SCRATCH_MEM",
};

State::State() { reset(); }

void State::reset() {
  nextLabelId = 0;
  func.clear();
  funcType.clear();
  argSize = 0;
  line = 0;
  scopeStack.clear();
  memVarMap.clear();
  outWarn.clear();
  outInfo.clear();
  funcMap.clear();
  barrierMaskMap.clear();
  regAllocAllowed = true;

  for (const auto &label : LABELS) {
    declareMemVar(label, "u16", 1);
  }
}

// --- Error handling ---------------------------------------------------

void State::throwError(const std::string &msg,
                       const std::string &context) const {
  std::ostringstream oss;
  oss << "Error in " << (func.empty() ? "(???)" : func) << ", line "
      << (line == 0 ? "(???)" : std::to_string(line)) << ": " << msg
      << "\n  -> AST: " << context;
  throw std::runtime_error(oss.str());
}

void State::logWarning(const std::string &msg, const std::string &context) {
  std::ostringstream oss;
  oss << "Warning in " << (func.empty() ? "(???)" : func) << ", line "
      << (line == 0 ? "(???)" : std::to_string(line)) << ": " << msg
      << "\n  -> AST: " << context << "\n";
  outWarn += oss.str();
}

void State::logInfo(const std::string &msg) { outInfo += msg + '\n'; }

// --- Function management ----------------------------------------------

void State::declareFunction(const std::string &name,
                            const std::vector<ast::FuncDefArg> &args,
                            bool isRelative) {
  funcMap[name] = {name, args, isRelative};
}

void State::enterFunction(const std::string &name, const std::string &type,
                          int argSize_) {
  func = name;
  funcType = type;
  argSize = argSize_ > 0 ? argSize_ : 0;
  line = 0;
  scopeStack.clear();
  pushScope();

  // Declare built-in registers as variables
  declareVar("ZERO", "u32", reg::Reg::ZERO, true);
  declareVar("VZERO", "vec16", reg::Reg::VZERO, true);
  declareVar("VSHIFT", "vec16", reg::Reg::VSHIFT, true);
  declareVar("VSHIFT8", "vec16", reg::Reg::VSHIFT8, true);
  declareVar("RA", "u32", reg::Reg::RA, false);
  declareVar("VTEMP", "vec16", reg::Reg::VTEMP0, false, true);
}

void State::leaveFunction() {
  func.clear();
  funcType.clear();
  line = 0;
  scopeStack.clear();
}

const FuncDef *State::getFunction(const std::string &name) const {
  auto it = funcMap.find(name);
  return it != funcMap.end() ? &it->second : nullptr;
}

// --- Scope management -------------------------------------------------

Scope &State::getScope() { return scopeStack.back(); }

void State::pushScope(const std::string &labelStart,
                      const std::string &labelEnd) {
  Scope child = makeChildScope();
  if (!labelStart.empty() || !labelEnd.empty()) {
    child.labelStart =
        labelStart.empty() ? child.labelStart : labelStart;
    child.labelEnd = labelEnd.empty() ? child.labelEnd : labelEnd;
  }
  scopeStack.push_back(std::move(child));
}

void State::popScope() { scopeStack.pop_back(); }

Scope State::makeChildScope() const {
  if (scopeStack.empty()) {
    return Scope{};
  }
  const auto &parent = scopeStack.back();
  return Scope{
      .varMap = parent.varMap,
      .regVarMap = parent.regVarMap,
      .varAliasMap = parent.varAliasMap,
      .annotations = parent.annotations,
      .labelStart = parent.labelStart,
      .labelEnd = parent.labelEnd,
  };
}

// --- Variable management ----------------------------------------------

void State::declareVar(const std::string &name, const std::string &type,
                       const std::string &reg, bool isConst,
                       bool ignoreReserved) {
  if (name.find(':') != std::string::npos) {
    throwError("Variable name cannot contain a cast (':')!", {name});
  }
  Scope &scope = getScope();
  if (reg.empty()) {
    throwError("Cannot declare variable without register!", {name});
  }
  if (!ignoreReserved &&
      std::find(reg::REGS_FORBIDDEN.begin(), reg::REGS_FORBIDDEN.end(),
                reg) != reg::REGS_FORBIDDEN.end()) {
    throwError("Cannot use reserved register '" + reg + "' for a variable!",
               {name});
  }

  if (isVecType(type)) {
    if (!reg::isVecReg(reg)) {
      throwError("Cannot use scalar register for vector variable!", {name});
    }
  } else {
    if (reg::isVecReg(reg)) {
      throwError("Cannot use vector register for scalar variable!", {name});
    }
  }

  // Check for double-allocation
  auto checkReg = [&](const std::string &r) {
    auto it = scope.regVarMap.find(r);
    if (it != scope.regVarMap.end()) {
      throwError("Register '" + r + "' already used for variable '" +
                     it->second + "'!",
                 {name});
    }
  };

  checkReg(reg);
  scope.varMap[name] = VarDef{reg, type, {}, {}, {}, {}, 0, isConst, 0};
  scope.regVarMap[reg] = name;

  if (isTwoRegType(type)) {
    const std::string *nextR = reg::nextReg(reg);
    if (!nextR) throwError("No next register for two-reg type!", {name});
    checkReg(*nextR);
    scope.regVarMap[*nextR] = name;
  }
}

void State::declareVarAlias(const std::string &aliasName,
                            const std::string &varName) {
  getRequiredVar(varName, "alias");
  Scope &scope = getScope();
  auto it = scope.varAliasMap.find(varName);
  const std::string &realName = (it != scope.varAliasMap.end())
                                    ? it->second
                                    : varName;
  scope.varAliasMap[aliasName] = realName;
}

void State::undefVar(const std::string &varName) {
  Scope &scope = getScope();

  scope.varAliasMap.erase(varName);
  std::vector<std::string> toErase;
  for (const auto &[alias, target] : scope.varAliasMap) {
    if (target == varName) toErase.push_back(alias);
  }
  for (const auto &a : toErase) {
    scope.varAliasMap.erase(a);
  }

  std::string resolved = varName;
  auto aliasIt = scope.varAliasMap.find(varName);
  if (aliasIt != scope.varAliasMap.end()) {
    resolved = aliasIt->second;
  }

  auto varIt = scope.varMap.find(resolved);
  if (varIt == scope.varMap.end()) {
    throwError("Variable " + resolved + " not known!");
  }

  // Free registers
  scope.regVarMap.erase(varIt->second.reg);
  if (isTwoRegType(varIt->second.type)) {
    const std::string *nextR = reg::nextReg(varIt->second.reg);
    if (nextR) scope.regVarMap.erase(*nextR);
  }
  scope.varMap.erase(varIt);
}

VarDef *State::getVar(const std::string &name) {
  Scope &scope = getScope();
  std::string nameNorm = name;
  auto colonPos = nameNorm.find(':');
  if (colonPos != std::string::npos) {
    nameNorm = nameNorm.substr(0, colonPos);
  }
  auto aliasIt = scope.varAliasMap.find(nameNorm);
  if (aliasIt != scope.varAliasMap.end()) {
    nameNorm = aliasIt->second;
  }
  auto it = scope.varMap.find(nameNorm);
  return it != scope.varMap.end() ? &it->second : nullptr;
}

const VarDef *State::getRequiredVar(const std::string &name,
                                     const std::string &contextName,
                                     const std::string &context) {
  VarDef *var = getVar(name);
  if (!var) {
    // Fallback: check memory variable map
    auto memIt = memVarMap.find(name);
    if (memIt != memVarMap.end()) {
      static thread_local VarDef memVar;
      memVar = VarDef{};
      memVar.type = memIt->second.type;
      memVar.name = memIt->second.name;
      memVar.reg = "%lo(" + memIt->second.name + ")"; // Use as label ref
      return &memVar;
    }
    throwError(contextName + " Variable " + name + " not known!", context);
  }
  return var;
}

VarDef State::getRequiredVarCopy(const std::string &name,
                                  const std::string &contextName,
                                  const std::string &context) {
  const VarDef *var = getRequiredVar(name, contextName, context);
  VarDef copy = *var;
  // Store the original variable name (without cast) for macro arg passing
  auto cp = name.find(':');
  copy.name = (cp != std::string::npos) ? name.substr(0, cp) : name;

  // Handle cast suffix
  auto colonPos = name.find(':');
  if (colonPos != std::string::npos) {
    std::string castType = name.substr(colonPos + 1);
    copy.originalType = copy.type;
    copy.castType = castType;

    if (isVecType(copy.type)) {
      if (std::find(VEC_CASTS.begin(), VEC_CASTS.end(), castType) ==
          VEC_CASTS.end()) {
        throwError("Invalid cast type '" + castType + "' for variable " +
                       name + ", expected: uint,sint,ufract,sfract!",
                   context);
      }
      if (copy.type == "vec32" &&
          (castType == "sfract" || castType == "ufract")) {
        const std::string *nextV = reg::nextVecReg(copy.reg);
        if (nextV) copy.reg = *nextV;
      }
      copy.type = "vec16";
    } else {
      if (std::find(SCALAR_TYPES.begin(), SCALAR_TYPES.end(), castType) ==
          SCALAR_TYPES.end()) {
        throwError(
            "Invalid cast type '" + castType + "' for variable " + name +
                ", expected: s8,u8,s16,u16,s32,u32",
            context);
      }
      copy.type = castType;
    }
  }
  return copy;
}

const std::string *State::getVarReg(const std::string &name) const {
  const Scope &scope = scopeStack.back();
  std::string nameNorm = name;
  auto colonPos = nameNorm.find(':');
  if (colonPos != std::string::npos) {
    nameNorm = nameNorm.substr(0, colonPos);
  }
  auto aliasIt = scope.varAliasMap.find(nameNorm);
  if (aliasIt != scope.varAliasMap.end()) {
    nameNorm = aliasIt->second;
  }
  auto it = scope.varMap.find(nameNorm);
  return it != scope.varMap.end() ? &it->second.reg : nullptr;
}

bool State::varExists(const std::string &name) const {
  const Scope &scope = scopeStack.back();
  std::string nameNorm = name;
  auto colonPos = nameNorm.find(':');
  if (colonPos != std::string::npos) {
    nameNorm = nameNorm.substr(0, colonPos);
  }
  auto aliasIt = scope.varAliasMap.find(nameNorm);
  if (aliasIt != scope.varAliasMap.end()) {
    nameNorm = aliasIt->second;
  }
  return scope.varMap.count(nameNorm) > 0;
}

void State::markVarModified(const std::string &name) {
  Scope &scope = getScope();
  std::string nameNorm = name;
  auto colonPos = nameNorm.find(':');
  if (colonPos != std::string::npos) {
    nameNorm = nameNorm.substr(0, colonPos);
  }
  auto aliasIt = scope.varAliasMap.find(nameNorm);
  if (aliasIt != scope.varAliasMap.end()) {
    nameNorm = aliasIt->second;
  }
  auto it = scope.varMap.find(nameNorm);
  if (it == scope.varMap.end()) {
    throwError("Variable " + name + " not known!");
  }
  it->second.modifyCount++;
}

// --- Memory variables -------------------------------------------------

void State::declareMemVar(const std::string &name, const std::string &type,
                          int arraySize) {
  memVarMap[name] = {name, type, arraySize};
}

const MemVarDef *State::getRequiredMem(const std::string &name,
                                        const std::string &contextName,
                                        const std::string &context) const {
  auto it = memVarMap.find(name);
  if (it == memVarMap.end()) {
    throwError(contextName + " Memory-Var " + name + " not known!", context);
  }
  return &it->second;
}

VarOrMem State::getRequiredVarOrMem(const std::string &name,
                                       const std::string &contextName,
                                       const std::string &context) const {
  const Scope &scope = scopeStack.back();

  // Check memory map first
  auto memIt = memVarMap.find(name);
  if (memIt != memVarMap.end()) {
    return {memIt->second.name, memIt->second.type, "",
            memIt->second.arraySize};
  }

  // Check variable scope
  std::string nameNorm = name;
  auto aliasIt = scope.varAliasMap.find(nameNorm);
  if (aliasIt != scope.varAliasMap.end()) {
    nameNorm = aliasIt->second;
  }
  auto varIt = scope.varMap.find(nameNorm);
  if (varIt != scope.varMap.end()) {
    return {varIt->first, varIt->second.type, varIt->second.reg, 1};
  }

  throwError(contextName + " Variable/Memory " + name + " not known!",
             context);
  return {}; // unreachable
}

// --- Register allocation ----------------------------------------------

std::string State::allocRegister(const std::string &type) {
  if (!regAllocAllowed) {
    throwError("Register allocation not allowed in this function!");
  }

  bool reverse = (funcType == "command");
  const auto &regList = isVecType(type) ? reg::REGS_ALLOC_VECTOR
                                        : reg::REGS_ALLOC_SCALAR;
  const Scope &scope = getScope();
  bool twoRegs = isTwoRegType(type);

  auto tryAlloc = [&](const std::string &reg) -> std::string {
    if (scope.regVarMap.count(reg)) return {};
    if (twoRegs) {
      const std::string *nextR = reg::nextReg(reg);
      if (!nextR ||
          std::find(regList.begin(), regList.end(), *nextR) ==
              regList.end() ||
          scope.regVarMap.count(*nextR)) {
        return {};
      }
    }
    return reg;
  };

  if (reverse) {
    for (auto it = regList.rbegin(); it != regList.rend(); ++it) {
      std::string found = tryAlloc(*it);
      if (!found.empty()) return found;
    }
  } else {
    for (const auto &reg : regList) {
      std::string found = tryAlloc(reg);
      if (!found.empty()) return found;
    }
  }

  throwError("Out of free registers!");
  return {}; // unreachable
}

// --- Labels -----------------------------------------------------------

std::string State::generateLabel() {
  ++nextLabelId;
  char buf[64];
  snprintf(buf, sizeof(buf), "LABEL_%s_%04X",
           func.c_str(), nextLabelId);
  return buf;
}

// --- Annotations ------------------------------------------------------

void State::addAnnotation(const std::string &name,
                          const std::string &value) {
  Scope &scope = getScope();
  scope.annotations.push_back({name, value});
}

std::vector<AnnotationDef> State::getAnnotations(
    const std::string &name) const {
  if (scopeStack.empty()) return {};
  const auto &annos = scopeStack.back().annotations;
  if (name.empty()) return annos;

  std::vector<AnnotationDef> result;
  for (const auto &a : annos) {
    if (a.name == name) result.push_back(a);
  }
  return result;
}

void State::clearAnnotations() {
  if (!scopeStack.empty()) {
    scopeStack.back().annotations.clear();
  }
}

// --- Barrier masks ----------------------------------------------------

uint32_t State::getBarrierMask(const std::string &name) {
  auto it = barrierMaskMap.find(name);
  if (it != barrierMaskMap.end()) return it->second;

  int len = barrierMaskMap.size();
  if (len >= 32) {
    throwError("Too many different barriers, only up to 32 are supported!");
  }
  uint32_t mask = (1u << len);
  barrierMaskMap[name] = mask;
  return mask;
}

// --- Global instance --------------------------------------------------

State state;

} // namespace rspl

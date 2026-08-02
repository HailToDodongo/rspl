#pragma once

#include "ast.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace rspl {

// --- Variable definition ----------------------------------------------

struct VarDef {
  std::string reg;
  TypeClass type = TypeClass::Unknown;
  std::string name;         // for memory label references
  TypeClass originalType = TypeClass::Unknown; // before cast
  CastType castType = CastType::None;         // e.g. ufract, sfract, s8
  std::string swizzle;      // optional swizzle suffix
  double value = 0.0;        // numeric value when reg is empty
  bool isConst = false;
  int modifyCount = 0;
  bool isGlobal = false;     // file-level register-pinned var, not undef-able

  // Backward-compat accessors for code that still uses strings
  std::string typeStr() const { return toString(type); }
  std::string originalTypeStr() const { return toString(originalType); }
  std::string castTypeStr() const { return toString(castType); }
};

struct MemVarDef {
  std::string name;
  std::string type;
  int arraySize = 1;
};

// Union type for getRequiredVarOrMem — either a register variable or
// a memory label. Check `reg` to see which.
struct VarOrMem {
  std::string name;
  std::string type;
  std::string reg;    // empty -> this is a memory variable
  int arraySize = 1;  // only set for memory variables
};

struct FuncDef {
  std::string name;
  std::vector<ast::FuncDefArg> args;
  bool isRelative = false;
};

struct AnnotationDef {
  std::string name;
  std::string value;
};

// --- Scope ------------------------------------------------------------

struct Scope {
  // Variable name -> definition (inherited from parent scope via copy-down)
  std::unordered_map<std::string, VarDef> varMap;
  // Register -> variable name (for collision detection)
  std::unordered_map<std::string, std::string> regVarMap;
  // Alias -> real variable name (for macro args)
  std::unordered_map<std::string, std::string> varAliasMap;
  // Scope-local annotations
  std::vector<AnnotationDef> annotations;
  // Label targets for break/continue
  std::string labelStart;
  std::string labelEnd;
};

// --- State ------------------------------------------------------------

class State {
public:
  State();

  // -- Reset -----------------------------------------------------------
  void reset();

  // -- Error / warning / info ------------------------------------------
  [[noreturn]] void throwError(const std::string &msg,
                                const std::string &context = "{}") const;

  void logWarning(const std::string &msg, const std::string &context = "{}");
  void logInfo(const std::string &msg);

  // -- Source tracking -------------------------------------------------
  std::vector<std::string> sourceLines;
  std::string func;       // current function name
  std::string funcType;   // "function", "command", "macro"
  int argSize = 0;
  uint32_t line = 0;
  std::string outWarn;
  std::string outInfo;

  // -- Function management ---------------------------------------------
  void declareFunction(const std::string &name,
                       const std::vector<ast::FuncDefArg> &args,
                       bool isRelative = false);
  void enterFunction(const std::string &name, const std::string &type,
                     int argSize);
  void leaveFunction();
  const FuncDef *getFunction(const std::string &name) const;

  // -- Scope management ------------------------------------------------
  Scope &getScope();
  void pushScope(const std::string &labelStart = "",
                 const std::string &labelEnd = "");
  void popScope();

  // -- Global register variables ---------------------------------------
  struct GlobalVarDef {
    std::string name;
    std::string type;
    std::string reg;
    bool isConst = false;
  };
  // Registered once per program; re-declared into every function's root
  // scope by enterFunction().
  void declareGlobalVar(const std::string &name, const std::string &type,
                        const std::string &reg, bool isConst);

  // -- Variable management ---------------------------------------------
  void declareVar(const std::string &name, const std::string &type,
                  const std::string &reg, bool isConst = false,
                  bool ignoreReserved = false);
  void declareVarAlias(const std::string &aliasName,
                       const std::string &varName);
  void undefVar(const std::string &varName);
  VarDef *getVar(const std::string &name);
  const VarDef *getRequiredVar(const std::string &name,
                                const std::string &contextName,
                                const std::string &context = "{}");
  const std::string *getVarReg(const std::string &name) const;
  bool varExists(const std::string &name) const;
  void markVarModified(const std::string &name);
  VarDef getRequiredVarCopy(const std::string &name,
                             const std::string &contextName,
                             const std::string &context = "{}");

  // -- Memory variables (global state labels) --------------------------
  void declareMemVar(const std::string &name, const std::string &type,
                     int arraySize);
  const MemVarDef *getRequiredMem(const std::string &name,
                                   const std::string &contextName,
                                   const std::string &context = "{}") const;
  const MemVarDef *getMemVarOrNull(const std::string &name) const;
  VarOrMem getRequiredVarOrMem(const std::string &name,
                                   const std::string &contextName,
                                   const std::string &context = "{}") const;

  // -- Register allocation ---------------------------------------------
  std::string allocRegister(const std::string &type);
  bool regAllocAllowed = true;

  // -- Labels ----------------------------------------------------------
  std::string generateLabel();

  // -- Annotations -----------------------------------------------------
  void addAnnotation(const std::string &name, const std::string &value,
                     bool valueIsString = true);
  std::vector<AnnotationDef> getAnnotations(
      const std::string &name = "") const;
  void clearAnnotations();

  // -- Barrier masks ---------------------------------------------------
  uint32_t getBarrierMask(const std::string &name);

private:
  int nextLabelId = 0;
  std::vector<GlobalVarDef> globalVars;
  std::vector<Scope> scopeStack;
  std::unordered_map<std::string, MemVarDef> memVarMap;
  std::unordered_map<std::string, FuncDef> funcMap;
  std::unordered_map<std::string, uint32_t> barrierMaskMap;

  Scope makeChildScope() const;
};

// Global state instance (mirrors JS `export default state`)
extern State state;

} // namespace rspl

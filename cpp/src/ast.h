#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace rspl::ast {

// --- Leaf / argument types --------------------------------------------

struct ExprNum {
  double value = 0.0;
};

struct ExprVarName {
  std::string value;
};

// --- Function arguments -----------------------------------------------

struct FuncArg {
  std::string type;   // "var", "num", or "string"
  std::string value;
  std::string swizzle; // optional, empty if absent
};

// --- Function definition argument -------------------------------------

struct FuncDefArg {
  std::string type; // data type e.g. "u32", "vec16"
  std::string reg;  // optional register constraint, e.g. "$t0"
  std::string name;
};

// --- Annotations ------------------------------------------------------

struct Annotation {
  std::string name;
  std::string value;
};

// --- Comparison expression (in if/while/loop conditions) --------------

struct CompareExpr {
  FuncArg left;
  std::string op;
  FuncArg right;
  uint32_t line = 0;
};

// --- Calculation types ------------------------------------------------

struct CalcNum {
  ExprNum right;
};

struct CalcVar {
  std::string op;     // "!" / "~" / empty
  ExprVarName right;
  std::string swizzleRight;
};

struct CalcLR {
  ExprVarName left;
  std::string op;
  ExprNum rightNum;          // filled when right is numeric
  std::string rightVarName;  // filled when right is a variable
  std::string swizzleLeft;
  std::string swizzleRight;
};

struct CalcMultiPart {
  std::string op;
  ExprVarName right;            // when right is a VarName
  std::optional<double> rightVal; // when right is a number
  std::string swizzleRight;
  int32_t groupStart = 0;
  int32_t groupEnd = 0;
};

struct CalcMulti {
  ExprVarName left;
  std::optional<double> leftVal; // when left is a number
  std::string swizzleLeft;
  std::vector<CalcMultiPart> parts;
  int32_t groupStart = 0;
};

struct CalcFunc {
  std::string funcName;
  std::vector<FuncArg> args;
  std::string swizzleRight;
};

struct TernaryPart {
  std::string left;              // variable name
  std::string right;             // variable name
  std::optional<double> rightVal; // value when right is a number
  std::string swizzleRight;
};

struct CalcCompare {
  std::string left;               // variable name
  std::string op;
  std::string right;              // variable name
  std::optional<double> rightVal; // value when right is a number
  std::string swizzleRight;
  std::optional<TernaryPart> ternary;
};

// Calc variant — includes all calculation node types
using Calc = std::variant<
    CalcNum,
    CalcVar,
    CalcLR,
    CalcMulti,
    CalcMultiPart,
    CalcFunc,
    CalcCompare
>;

// --- Statement types --------------------------------------------------

struct StmtVarDecl {
  std::string varName;
  std::string varType;
  std::string reg;
  bool isConst = false;
  uint32_t line = 0;
};

struct StmtVarDeclMulti {
  std::string varType;
  std::string reg;
  std::vector<std::string> varNames;
  bool isConst = false;
  uint32_t line = 0;
};

struct StmtVarDeclAssign {
  std::string varType;
  std::string reg;
  std::string varName;
  std::unique_ptr<Calc> calc;
  bool isConst = false;
  uint32_t line = 0;
};

struct StmtVarDeclAlias {
  std::string aliasName;
  std::string varName;
};

struct StmtVarUndef {
  std::string varName;
  uint32_t line = 0;
};

struct StmtVarAssignCalc {
  std::string varName;
  std::string swizzle;
  std::string assignType; // "=", "+=", "-=", etc.
  std::unique_ptr<Calc> calc;
  uint32_t line = 0;
};

struct StmtFuncCall {
  std::string func;
  std::vector<FuncArg> args;
  uint32_t line = 0;
};

struct StmtLabelDecl {
  std::string name;
  uint32_t line = 0;
};

struct StmtGoto {
  std::string label;
  uint32_t line = 0;
};

struct StmtIf {
  CompareExpr compare;
  std::unique_ptr<struct ScopedBlock> blockIf;
  std::unique_ptr<struct ScopedBlock> blockElse;
  uint32_t line = 0;
};

struct StmtWhile {
  CompareExpr compare;
  std::unique_ptr<struct ScopedBlock> block;
  uint32_t line = 0;
};

struct StmtLoop {
  std::optional<CompareExpr> compare;
  std::unique_ptr<struct ScopedBlock> block;
  uint32_t line = 0;
};

struct StmtBreak {
  uint32_t line = 0;
};

struct StmtContinue {
  uint32_t line = 0;
};

struct StmtExit {
  uint32_t line = 0;
};

struct StmtAnnotation {
  std::string name;
  std::string value;
  uint32_t line = 0;
};

// --- Stmt variant + ScopedBlock (mutually recursive) ------------------

// Nested scoped block — carries its own scope
struct StmtScopedBlock {
  std::unique_ptr<struct ScopedBlock> body;
};

using Stmt = std::variant<
    StmtVarDecl,
    StmtVarDeclMulti,
    StmtVarDeclAssign,
    StmtVarDeclAlias,
    StmtVarUndef,
    StmtVarAssignCalc,
    StmtFuncCall,
    StmtLabelDecl,
    StmtGoto,
    StmtIf,
    StmtWhile,
    StmtLoop,
    StmtBreak,
    StmtContinue,
    StmtExit,
    StmtAnnotation,
    StmtScopedBlock
>;

struct ScopedBlock {
  std::vector<Stmt> statements;
  uint32_t line = 0;
};

// --- State variable definition ----------------------------------------

struct StateVarDef {
  std::string varType;
  std::string varName;
  bool isExtern = false;
  std::vector<int64_t> arraySize;
  int64_t align = 0;
  std::vector<int64_t> value;
};

// --- State section ----------------------------------------------------

struct StateSection {
  std::string name; // "state", "data", "bss"
  std::vector<StateVarDef> vars;
};

// --- Function ---------------------------------------------------------

struct Function {
  std::vector<Annotation> annotations;
  std::string type;           // "function", "command", "macro"
  std::optional<int64_t> resultType; // command index
  std::string name;
  std::vector<FuncDefArg> args;
  std::unique_ptr<ScopedBlock> body; // null for extern declarations
};

// --- Top-level AST ----------------------------------------------------

struct DefineEntry {
  std::string name;
  std::string value;
};

struct Program {
  std::vector<std::string> includes;
  std::vector<StateSection> states;
  std::vector<Function> functions;
  std::vector<std::string> postIncludes;
  std::vector<DefineEntry> defines;
};

// --- JSON deserialization ---------------------------------------------

Program parseJson(const std::string &json);

} // namespace rspl::ast

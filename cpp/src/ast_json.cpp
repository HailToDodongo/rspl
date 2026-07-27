#include "ast_json.h"

#include <nlohmann/json.hpp>

namespace rspl {

using json = nlohmann::ordered_json;

namespace {

json serializeFuncArg(const ast::FuncArg &a) {
  return {{"type", toString(a.type)},
          {"value", a.value},
          {"swizzle", a.swizzle}};
}

json serializeCompare(const ast::CompareExpr &c) {
  return {{"left", serializeFuncArg(c.left)},
          {"op", c.op},
          {"right", serializeFuncArg(c.right)},
          {"line", c.line}};
}

json serializeCalc(const ast::Calc &calc);

json serializeCalcMultiPart(const ast::CalcMultiPart &p) {
  json j = {{"op", p.op},
            {"swizzleRight", p.swizzleRight},
            {"groupStart", p.groupStart},
            {"groupEnd", p.groupEnd}};
  if (p.rightVal.has_value()) {
    j["rightVal"] = *p.rightVal;
  } else {
    j["right"] = p.right.value;
  }
  return j;
}

json serializeCalc(const ast::Calc &calc) {
  return std::visit(
      [](const auto &c) -> json {
        using T = std::decay_t<decltype(c)>;
        if constexpr (std::is_same_v<T, ast::CalcNum>) {
          return {{"type", "calcNum"}, {"right", c.right.value}};
        } else if constexpr (std::is_same_v<T, ast::CalcVar>) {
          return {{"type", "calcVar"},
                  {"op", c.op},
                  {"right", c.right.value},
                  {"swizzleRight", c.swizzleRight}};
        } else if constexpr (std::is_same_v<T, ast::CalcLR>) {
          return {{"type", "calcLR"},
                  {"left", c.left.value},
                  {"op", c.op},
                  {"rightNum", c.rightNum.value},
                  {"rightVarName", c.rightVarName},
                  {"swizzleLeft", c.swizzleLeft},
                  {"swizzleRight", c.swizzleRight}};
        } else if constexpr (std::is_same_v<T, ast::CalcMulti>) {
          json j = {{"type", "calcMulti"},
                    {"swizzleLeft", c.swizzleLeft},
                    {"groupStart", c.groupStart}};
          if (c.leftVal.has_value()) {
            j["leftVal"] = *c.leftVal;
          } else {
            j["left"] = c.left.value;
          }
          json parts = json::array();
          for (const auto &p : c.parts) parts.push_back(serializeCalcMultiPart(p));
          j["parts"] = std::move(parts);
          return j;
        } else if constexpr (std::is_same_v<T, ast::CalcMultiPart>) {
          return serializeCalcMultiPart(c);
        } else if constexpr (std::is_same_v<T, ast::CalcFunc>) {
          json args = json::array();
          for (const auto &a : c.args) args.push_back(serializeFuncArg(a));
          return {{"type", "calcFunc"},
                  {"funcName", c.funcName},
                  {"args", std::move(args)},
                  {"swizzleRight", c.swizzleRight}};
        } else if constexpr (std::is_same_v<T, ast::CalcCompare>) {
          json j = {{"type", "calcCompare"},
                    {"left", c.left},
                    {"op", c.op},
                    {"swizzleRight", c.swizzleRight}};
          if (c.rightVal.has_value()) {
            j["rightVal"] = *c.rightVal;
          } else {
            j["right"] = c.right;
          }
          if (c.ternary.has_value()) {
            json t = {{"left", c.ternary->left},
                      {"swizzleRight", c.ternary->swizzleRight}};
            if (c.ternary->rightVal.has_value()) {
              t["rightVal"] = *c.ternary->rightVal;
            } else {
              t["right"] = c.ternary->right;
            }
            j["ternary"] = std::move(t);
          }
          return j;
        }
      },
      calc);
}

json serializeBlock(const ast::ScopedBlock &block);
json serializeFunction(const ast::Function &fn);

json serializeStmt(const ast::Stmt &stmt) {
  return std::visit(
      [](const auto &s) -> json {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, ast::StmtVarDecl>) {
          return {{"type", "varDecl"},   {"varName", s.varName},
                  {"varType", s.varType}, {"reg", s.reg},
                  {"isConst", s.isConst}, {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtVarDeclMulti>) {
          return {{"type", "varDeclMulti"}, {"varType", s.varType},
                  {"reg", s.reg},           {"varNames", s.varNames},
                  {"isConst", s.isConst},   {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtVarDeclAssign>) {
          json j = {{"type", "varDeclAssign"}, {"varType", s.varType},
                    {"reg", s.reg},            {"varName", s.varName},
                    {"isConst", s.isConst},    {"line", s.line}};
          j["calc"] = s.calc ? serializeCalc(*s.calc) : json(nullptr);
          return j;
        } else if constexpr (std::is_same_v<T, ast::StmtVarDeclAlias>) {
          return {{"type", "varDeclAlias"},
                  {"aliasName", s.aliasName},
                  {"varName", s.varName},
                  {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtVarUndef>) {
          return {{"type", "varUndef"}, {"varName", s.varName}, {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtVarAssignCalc>) {
          json j = {{"type", "varAssignCalc"}, {"varName", s.varName},
                    {"swizzle", s.swizzle},    {"assignType", s.assignType},
                    {"line", s.line}};
          j["calc"] = s.calc ? serializeCalc(*s.calc) : json(nullptr);
          return j;
        } else if constexpr (std::is_same_v<T, ast::StmtFuncCall>) {
          json args = json::array();
          for (const auto &a : s.args) args.push_back(serializeFuncArg(a));
          return {{"type", "funcCall"},
                  {"func", s.func},
                  {"args", std::move(args)},
                  {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtLabelDecl>) {
          return {{"type", "labelDecl"}, {"name", s.name}, {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtGoto>) {
          return {{"type", "goto"}, {"label", s.label}, {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtIf>) {
          json j = {{"type", "if"},
                    {"compare", serializeCompare(s.compare)},
                    {"line", s.line}};
          j["blockIf"] = s.blockIf ? serializeBlock(*s.blockIf) : json(nullptr);
          j["blockElse"] =
              s.blockElse ? serializeBlock(*s.blockElse) : json(nullptr);
          return j;
        } else if constexpr (std::is_same_v<T, ast::StmtWhile>) {
          json j = {{"type", "while"},
                    {"compare", serializeCompare(s.compare)},
                    {"line", s.line}};
          j["block"] = s.block ? serializeBlock(*s.block) : json(nullptr);
          return j;
        } else if constexpr (std::is_same_v<T, ast::StmtLoop>) {
          json j = {{"type", "loop"}, {"line", s.line}};
          j["compare"] = s.compare.has_value() ? serializeCompare(*s.compare)
                                               : json(nullptr);
          j["block"] = s.block ? serializeBlock(*s.block) : json(nullptr);
          return j;
        } else if constexpr (std::is_same_v<T, ast::StmtBreak>) {
          return {{"type", "break"}, {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtContinue>) {
          return {{"type", "continue"}, {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtExit>) {
          return {{"type", "exit"}, {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtAnnotation>) {
          return {{"type", "annotation"},
                  {"name", s.name},
                  {"value", s.value},
                  {"valueIsString", s.valueIsString},
                  {"line", s.line}};
        } else if constexpr (std::is_same_v<T, ast::StmtScopedBlock>) {
          json j = {{"type", "scopedBlock"}, {"line", s.line}};
          j["body"] = s.body ? serializeBlock(*s.body) : json(nullptr);
          return j;
        } else if constexpr (std::is_same_v<T, ast::StmtMacroDef>) {
          // C++-only extension — never produced by the JS parser
          json j = {{"type", "macroDef"}, {"line", s.line}};
          j["def"] = s.def ? serializeFunction(*s.def) : json(nullptr);
          return j;
        }
      },
      stmt);
}

json serializeBlock(const ast::ScopedBlock &block) {
  json stmts = json::array();
  for (const auto &s : block.statements) stmts.push_back(serializeStmt(s));
  return {{"line", block.line}, {"statements", std::move(stmts)}};
}

json serializeFunction(const ast::Function &fn) {
  json annos = json::array();
  for (const auto &a : fn.annotations) {
    annos.push_back({{"name", a.name},
                     {"value", a.value},
                     {"valueIsString", a.valueIsString}});
  }
  json args = json::array();
  for (const auto &arg : fn.args) {
    args.push_back({{"type", toString(arg.type)},
                    {"reg", arg.reg},
                    {"name", arg.name}});
  }
  json jf = {{"type", toString(fn.type)},
             {"name", fn.name},
             {"hasResultType", fn.hasResultType},
             {"annotations", std::move(annos)},
             {"args", std::move(args)}};
  jf["resultType"] =
      fn.resultType.has_value() ? json(*fn.resultType) : json(nullptr);
  jf["body"] = fn.body ? serializeBlock(*fn.body) : json(nullptr);
  return jf;
}

json serializeStateVar(const ast::StateVarDef &sv) {
  return {{"varType", sv.varType}, {"varName", sv.varName},
          {"extern", sv.isExtern}, {"arraySize", sv.arraySize},
          {"align", sv.align},     {"value", sv.value}};
}

} // namespace

std::string astToJson(const ast::Program &prog, bool pretty) {
  json j;
  j["includes"] = prog.includes;

  json states = json::array();
  for (const auto &sec : prog.states) {
    json vars = json::array();
    for (const auto &v : sec.vars) vars.push_back(serializeStateVar(v));
    states.push_back({{"name", sec.name}, {"vars", std::move(vars)}});
  }
  j["states"] = std::move(states);

  json uniforms = json::array();
  for (const auto &u : prog.uniforms) {
    json vars = json::array();
    for (const auto &v : u.state) vars.push_back(serializeStateVar(v));
    json ju = {{"name", u.name}, {"line", u.line}, {"state", std::move(vars)}};
    ju["binding"] = u.binding.has_value() ? json(*u.binding) : json(nullptr);
    uniforms.push_back(std::move(ju));
  }
  j["uniforms"] = std::move(uniforms);

  json attributes = json::array();
  for (const auto &a : prog.attributes) {
    json ja = {{"name", a.name},           {"type", a.type},
               {"arraySize", a.arraySize}, {"optional", a.optional},
               {"line", a.line}};
    ja["binding"] = a.binding.has_value() ? json(*a.binding) : json(nullptr);
    attributes.push_back(std::move(ja));
  }
  j["attributes"] = std::move(attributes);

  json functions = json::array();
  for (const auto &fn : prog.functions) {
    functions.push_back(serializeFunction(fn));
  }
  j["functions"] = std::move(functions);

  j["postIncludes"] = prog.postIncludes;

  json defines = json::array();
  for (const auto &d : prog.defines) {
    defines.push_back({{"name", d.name}, {"value", d.value}});
  }
  j["defines"] = std::move(defines);

  return pretty ? j.dump(2) : j.dump();
}

} // namespace rspl

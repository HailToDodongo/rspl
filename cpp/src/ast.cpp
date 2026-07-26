#include "ast.h"

#include <nlohmann/json.hpp>
#include <stdexcept>

namespace rspl::ast {

using json = nlohmann::json;

// --- Forward declarations for recursive deserializers -----------------

static ScopedBlock parseScopedBlock(const json &j);
static Calc parseCalc(const json &j);
static CompareExpr parseCompareExpr(const json &j);

// --- Helpers ----------------------------------------------------------

static inline std::string optStr(const json &j, const char *key) {
  auto it = j.find(key);
  if (it == j.end() || it->is_null()) return {};
  if (it->is_string()) return it->get<std::string>();
  if (it->is_number()) return std::to_string(it->get<int64_t>());
  if (it->is_boolean()) return it->get<bool>() ? "true" : "false";
  return {};
}

static inline std::string jsonAsStr(const json &j) {
  if (j.is_string()) return j.get<std::string>();
  if (j.is_number()) return std::to_string(j.get<int64_t>());
  if (j.is_boolean()) return j.get<bool>() ? "true" : "false";
  return j.dump();
}

// Some grammar rules hand through the raw lexer token instead of its text
// (the JS side relies on the token's implicit toString()).
static inline std::string tokenStr(const json &j) {
  if (j.is_object()) {
    auto it = j.find("value");
    if (it != j.end() && it->is_string()) return it->get<std::string>();
  }
  return jsonAsStr(j);
}

static inline uint32_t tokenLine(const json &j) {
  if (j.is_object()) {
    auto it = j.find("line");
    if (it != j.end() && it->is_number()) return it->get<uint32_t>();
  }
  return 0;
}

static inline uint32_t optLine(const json &j) {
  auto it = j.find("line");
  if (it == j.end() || it->is_null()) return 0;
  if (it->is_number()) return it->get<uint32_t>();
  return 0;
}

// --- Leaf types -------------------------------------------------------

static ExprNum parseExprNum(const json &j) {
  return ExprNum{j.value("value", 0)};
}

static ExprVarName parseExprVarName(const json &j) {
  return ExprVarName{j.value("value", "")};
}

// --- FuncArg ----------------------------------------------------------

static FuncArg parseFuncArg(const json &j) {
  return FuncArg{
      .type = toArgType(j.value("type", "")),
      .value = jsonAsStr(j["value"]),
      .swizzle = optStr(j, "swizzle"),
  };
}

// --- Annotation -------------------------------------------------------

static bool annoValueIsString(const json &j) {
  auto it = j.find("value");
  return it != j.end() && it->is_string();
}

static Annotation parseAnnotation(const json &j) {
  return Annotation{
      .name = j.value("name", ""),
      .value = optStr(j, "value"),
      .valueIsString = annoValueIsString(j),
  };
}

// --- FuncDefArg -------------------------------------------------------

static FuncDefArg parseFuncDefArg(const json &j) {
  return FuncDefArg{
      .type = toTypeClass(j.value("type", "")),
      .reg = optStr(j, "reg"),
      .name = j.value("name", ""),
  };
}

// --- CalcParse -> dispatches on "type" field ---------------------------

static Calc parseCalc(const json &j) {
  std::string type = j.value("type", "");
  if (type == "calcNum") {
    // calcNum.right is a plain number, not an object
    CalcNum cn;
    if (j["right"].is_number()) {
      cn.right = ExprNum{j["right"].get<double>()};
    } else if (j["right"].is_object()) {
      cn.right = parseExprNum(j["right"]);
    }
    return cn;
  }
  if (type == "calcVar") {
    return CalcVar{
        .op = optStr(j, "op"),
        .right = parseExprVarName(j["right"]),
        .swizzleRight = optStr(j, "swizzleRight"),
    };
  }
  if (type == "calcLR") {
    CalcLR lr;
    lr.left = parseExprVarName(j["left"]);
    lr.op = j.value("op", "");
    lr.swizzleLeft = optStr(j, "swizzleLeft");
    lr.swizzleRight = optStr(j, "swizzleRight");
    // right can be VarName or num
    if (j["right"].is_object() && j["right"].value("type", "") == "num") {
      lr.rightNum = ExprNum{j["right"].value("value", 0)};
    } else if (j["right"].is_object()) {
      lr.rightVarName = j["right"].value("value", "");
    }
    return lr;
  }
  if (type == "calcMulti") {
    CalcMulti cm;
    if (j["left"].is_object() && j["left"].value("type", "") == "num") {
      cm.leftVal = j["left"].value("value", int64_t{0});
    } else {
      cm.left = parseExprVarName(j["left"]);
    }
    cm.swizzleLeft = optStr(j, "swizzleLeft");
    cm.groupStart = j.value("groupStart", 0);
    if (j.contains("parts") && j["parts"].is_array()) {
      for (const auto &p : j["parts"]) {
        CalcMultiPart part;
        part.op = p.value("op", "");
        part.swizzleRight = optStr(p, "swizzleRight");
        part.groupStart = p.value("groupStart", 0);
        part.groupEnd = p.value("groupEnd", 0);
        if (p["right"].is_object() && p["right"].value("type", "") == "num") {
          part.rightVal = p["right"].value("value", int64_t{0});
        } else {
          part.right = parseExprVarName(p["right"]);
        }
        cm.parts.push_back(std::move(part));
      }
    }
    return cm;
  }
  if (type == "calcFunc") {
    CalcFunc cf;
    cf.funcName = j.value("funcName", "");
    cf.swizzleRight = optStr(j, "swizzleRight");
    if (j.contains("args") && j["args"].is_array()) {
      for (const auto &a : j["args"]) {
        cf.args.push_back(parseFuncArg(a));
      }
    }
    return cf;
  }
  if (type == "calcCompare") {
    CalcCompare cc;
    cc.left = j.value("left", "");
    cc.op = j.value("op", "");
    cc.swizzleRight = optStr(j, "swizzleRight");
    if (j["right"].is_number()) {
      cc.rightVal = j["right"].get<double>();
    } else if (j["right"].is_object() && j["right"].value("type", "") == "num") {
      cc.rightVal = j["right"].value("value", 0.0);
    } else if (j["right"].is_object()) {
      cc.right = j["right"].value("value", "");
    } else if (j["right"].is_string()) {
      cc.right = j["right"].get<std::string>();
    }
    if (j.contains("ternary") && !j["ternary"].is_null()) {
      TernaryPart tp;
      tp.left = j["ternary"].value("left", "");
      tp.swizzleRight = optStr(j["ternary"], "swizzleRight");
      if (j["ternary"]["right"].is_number()) {
        tp.rightVal = j["ternary"]["right"].get<double>();
      } else if (j["ternary"]["right"].is_object() &&
                 j["ternary"]["right"].value("type", "") == "num") {
        tp.rightVal = j["ternary"]["right"].value("value", 0.0);
      } else if (j["ternary"]["right"].is_object()) {
        tp.right = j["ternary"]["right"].value("value", "");
      } else if (j["ternary"]["right"].is_string()) {
        tp.right = j["ternary"]["right"].get<std::string>();
      }
      cc.ternary = std::move(tp);
    }
    return cc;
  }
  throw std::runtime_error("Unknown calc type: " + type);
}

// --- CompareExpr ------------------------------------------------------

static CompareExpr parseCompareExpr(const json &j) {
  return CompareExpr{
      .left = parseFuncArg(j["left"]),
      .op = j.value("op", ""),
      .right = parseFuncArg(j["right"]),
      .line = optLine(j),
  };
}

// --- ScopedBlock ------------------------------------------------------

static ScopedBlock parseScopedBlock(const json &j) {
  ScopedBlock block;
  block.line = optLine(j);
  if (j.contains("statements") && j["statements"].is_array()) {
    for (const auto &st : j["statements"]) {
      std::string stType = st.value("type", "");
      if (stType == "varDecl") {
        block.statements.push_back(StmtVarDecl{
            .varName = st.value("varName", ""),
            .varType = st.value("varType", ""),
            .reg = optStr(st, "reg"),
            .isConst = st.value("isConst", false),
            .line = optLine(st),
        });
      } else if (stType == "varDeclMulti") {
        StmtVarDeclMulti s;
        s.varType = st.value("varType", "");
        s.reg = optStr(st, "reg");
        s.isConst = st.value("isConst", false);
        s.line = optLine(st);
        if (st.contains("varNames") && st["varNames"].is_array()) {
          for (const auto &vn : st["varNames"]) {
            s.varNames.push_back(vn.is_string() ? vn.get<std::string>()
                               : vn.is_number() ? std::to_string(vn.get<int64_t>())
                                                : "");
          }
        }
        block.statements.push_back(std::move(s));
      } else if (stType == "varDeclAssign") {
        StmtVarDeclAssign s;
        s.varType = st.value("varType", "");
        s.reg = optStr(st, "reg");
        s.varName = st.value("varName", "");
        s.isConst = st.value("isConst", false);
        s.line = optLine(st);
        if (st.contains("calc") && !st["calc"].is_null()) {
          s.calc = std::make_unique<Calc>(parseCalc(st["calc"]));
        }
        block.statements.push_back(std::move(s));
      } else if (stType == "varUndef") {
        block.statements.push_back(StmtVarUndef{
            .varName = st.value("varName", ""),
            .line = optLine(st),
        });
      } else if (stType == "varAssignCalc") {
        StmtVarAssignCalc s;
        s.varName = st.value("varName", "");
        s.swizzle = optStr(st, "swizzle");
        s.assignType = st.value("assignType", "=");
        s.line = optLine(st);
        if (st.contains("calc") && !st["calc"].is_null()) {
          s.calc = std::make_unique<Calc>(parseCalc(st["calc"]));
        }
        block.statements.push_back(std::move(s));
      } else if (stType == "funcCall") {
        StmtFuncCall s;
        s.func = st.value("func", "");
        s.line = optLine(st);
        if (st.contains("args") && st["args"].is_array()) {
          for (const auto &a : st["args"]) {
            s.args.push_back(parseFuncArg(a));
          }
        }
        block.statements.push_back(std::move(s));
      } else if (stType == "labelDecl") {
        block.statements.push_back(StmtLabelDecl{
            .name = st.value("name", ""),
            .line = optLine(st),
        });
      } else if (stType == "goto") {
        block.statements.push_back(StmtGoto{
            .label = st.value("label", ""),
            .line = optLine(st),
        });
      } else if (stType == "if") {
        StmtIf s;
        s.compare = parseCompareExpr(st["compare"]);
        s.line = optLine(st);
        if (st.contains("blockIf") && !st["blockIf"].is_null()) {
          s.blockIf = std::make_unique<ScopedBlock>(parseScopedBlock(st["blockIf"]));
        }
        if (st.contains("blockElse") && !st["blockElse"].is_null()) {
          s.blockElse =
              std::make_unique<ScopedBlock>(parseScopedBlock(st["blockElse"]));
        }
        block.statements.push_back(std::move(s));
      } else if (stType == "while") {
        StmtWhile s;
        s.compare = parseCompareExpr(st["compare"]);
        s.line = optLine(st);
        if (st.contains("block") && !st["block"].is_null()) {
          s.block = std::make_unique<ScopedBlock>(parseScopedBlock(st["block"]));
        }
        block.statements.push_back(std::move(s));
      } else if (stType == "loop") {
        StmtLoop s;
        s.line = optLine(st);
        if (st.contains("compare") && !st["compare"].is_null()) {
          s.compare = parseCompareExpr(st["compare"]);
        }
        if (st.contains("block") && !st["block"].is_null()) {
          s.block = std::make_unique<ScopedBlock>(parseScopedBlock(st["block"]));
        }
        block.statements.push_back(std::move(s));
      } else if (stType == "break") {
        block.statements.push_back(StmtBreak{optLine(st)});
      } else if (stType == "continue") {
        block.statements.push_back(StmtContinue{optLine(st)});
      } else if (stType == "exit") {
        block.statements.push_back(StmtExit{optLine(st)});
      } else if (stType == "annotation") {
        StmtAnnotation s;
        s.name = st.value("name", "");
        s.value = st.contains("value") && !st["value"].is_null()
                      ? st["value"].is_string() ? st["value"].get<std::string>()
                                                : st["value"].dump()
                      : "";
        s.valueIsString = annoValueIsString(st);
        s.line = optLine(st);
        block.statements.push_back(std::move(s));
      } else if (stType == "scopedBlock") {
        // Keep as nested scoped block — scope boundaries matter for registers
        StmtScopedBlock sb;
        sb.line = optLine(st);
        sb.body = std::make_unique<ScopedBlock>(parseScopedBlock(st));
        block.statements.push_back(std::move(sb));
      } else if (stType == "nestedCalc") {
        // Flatten nestedCalc (synthetic node, not a real scope)
        ScopedBlock nested = parseScopedBlock(st);
        for (auto &ns : nested.statements) {
          block.statements.push_back(std::move(ns));
        }
      } else if (stType == "varDeclAlias") {
        StmtVarDeclAlias s;
        s.line = optLine(st);
        s.aliasName = st.value("aliasName", "");
        s.varName = st.value("varName", "");
        block.statements.push_back(std::move(s));
      } else if (stType == "nestedCalc") {
        // Synthetic node from astCalcNormalizer — contains its own
        // scopedBlock-like statements. Flatten them in.
        ScopedBlock nested = parseScopedBlock(st);
        for (auto &ns : nested.statements) {
          block.statements.push_back(std::move(ns));
        }
      } else {
        // Unknown statement type — skip with warning
        fprintf(stderr, "Warning: unknown statement type '%s', skipping\n",
                stType.c_str());
      }
    }
  }
  return block;
}

// --- State section ----------------------------------------------------

static StateVarDef parseStateVarDef(const json &j) {
  StateVarDef sv;
  sv.varType = j.value("varType", "");
  sv.varName = j.value("varName", "");
  sv.isExtern = j.value("extern", false);
  sv.align = j.value("align", int64_t{0});
  if (j.contains("arraySize") && j["arraySize"].is_array()) {
    for (const auto &a : j["arraySize"]) {
      sv.arraySize.push_back(a.get<int64_t>());
    }
  }
  if (j.contains("value") && j["value"].is_array()) {
    for (const auto &v : j["value"]) {
      sv.value.push_back(v.get<int64_t>());
    }
  }
  return sv;
}

static StateSection parseStateSection(const json &j) {
  StateSection sec;
  sec.name = j.value("name", "");
  if (j.contains("vars") && j["vars"].is_array()) {
    for (const auto &v : j["vars"]) {
      sec.vars.push_back(parseStateVarDef(v));
    }
  }
  return sec;
}

// --- Magma uniform / vertex attribute ---------------------------------

static Uniform parseUniform(const json &j) {
  Uniform u;
  if (j.contains("name")) {
    u.name = tokenStr(j["name"]);
    u.line = tokenLine(j["name"]);
  }
  if (j.contains("binding") && !j["binding"].is_null()) {
    u.binding = j["binding"].get<int64_t>();
  }
  if (j.contains("state") && j["state"].is_array()) {
    for (const auto &s : j["state"]) {
      u.state.push_back(parseStateVarDef(s));
    }
  }
  return u;
}

static Attribute parseAttribute(const json &j) {
  Attribute a;
  if (j.contains("name")) {
    a.name = tokenStr(j["name"]);
    a.line = tokenLine(j["name"]);
  }
  if (j.contains("binding") && !j["binding"].is_null()) {
    a.binding = j["binding"].get<int64_t>();
  }
  if (j.contains("type")) a.type = tokenStr(j["type"]);
  if (j.contains("arraySize") && j["arraySize"].is_array()) {
    for (const auto &s : j["arraySize"]) {
      if (s.is_number()) a.arraySize.push_back(s.get<int64_t>());
    }
  }
  a.optional = j.value("optional", false);
  return a;
}

// --- Function ---------------------------------------------------------

static Function parseFunction(const json &j) {
  Function func;
  if (j.contains("annotations") && j["annotations"].is_array()) {
    for (const auto &a : j["annotations"]) {
      func.annotations.push_back(parseAnnotation(a));
    }
  }
  func.type = toFuncType(j.value("type", "function"));
  if (j.contains("resultType") && !j["resultType"].is_null()) {
    func.hasResultType = true;
    if (j["resultType"].is_number()) {
      func.resultType = j["resultType"].get<int64_t>();
    }
  }
  func.name = j.value("name", "");
  if (j.contains("args") && j["args"].is_array()) {
    for (const auto &a : j["args"]) {
      func.args.push_back(parseFuncDefArg(a));
    }
  }
  if (j.contains("body") && !j["body"].is_null()) {
    func.body = std::make_unique<ScopedBlock>(parseScopedBlock(j["body"]));
  }
  return func;
}

// --- Program (top-level) ----------------------------------------------

Program parseJson(const std::string &jsonStr) {
  json j = json::parse(jsonStr);
  Program prog;

  if (j.contains("includes") && j["includes"].is_array()) {
    for (const auto &inc : j["includes"]) {
      prog.includes.push_back(inc.get<std::string>());
    }
  }
  if (j.contains("states") && j["states"].is_array()) {
    for (const auto &s : j["states"]) {
      prog.states.push_back(parseStateSection(s));
    }
  }
  if (j.contains("uniforms") && j["uniforms"].is_array()) {
    for (const auto &u : j["uniforms"]) {
      prog.uniforms.push_back(parseUniform(u));
    }
  }
  if (j.contains("attributes") && j["attributes"].is_array()) {
    for (const auto &a : j["attributes"]) {
      prog.attributes.push_back(parseAttribute(a));
    }
  }
  if (j.contains("functions") && j["functions"].is_array()) {
    for (const auto &f : j["functions"]) {
      prog.functions.push_back(parseFunction(f));
    }
  }
  if (j.contains("postIncludes") && j["postIncludes"].is_array()) {
    for (const auto &inc : j["postIncludes"]) {
      prog.postIncludes.push_back(inc.get<std::string>());
    }
  }
  if (j.contains("defines") && j["defines"].is_object()) {
    for (const auto &[name, def] : j["defines"].items()) {
      DefineEntry entry;
      entry.name = name;
      entry.value = def.contains("value") ? def["value"].get<std::string>() : def.get<std::string>();
      prog.defines.push_back(entry);
    }
  }

  return prog;
}

} // namespace rspl::ast

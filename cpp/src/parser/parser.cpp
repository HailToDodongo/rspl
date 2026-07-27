#include "parser.h"
#include "lexer.h"

#include <cmath>
#include <stdexcept>

namespace rspl::parser {

// Recursive-descent parser mirroring src/lib/grammar.ne rule for rule.
//
// Whitespace notes: the grammar writes optional whitespace (`_`) explicitly,
// so wherever it is ABSENT two tokens must be adjacent. This is observable
// and enforced here via Token::spaceBefore — e.g. `a = 1 ;` is a syntax
// error (Expression requires `;` adjacent) while `u32 A ;` in a state
// block is fine (StateVarDef has `_ %StmEnd`). `__` (required whitespace)
// appears only after `const`.
//
// Numeric notes: the JSON bridge (ast.cpp parseJson) converts some numbers
// through int64 (truncating floats) and keeps others as double. Each
// construction site below matches what parseJson produces for that field.

namespace {

// JSON.stringify-style number formatting (integral values have no ".0")
std::string jsNumToString(double v) {
  if (v == std::floor(v) && std::abs(v) < 9.2e18) {
    return std::to_string(static_cast<int64_t>(v));
  }
  char buf[32];
  snprintf(buf, sizeof(buf), "%.17g", v);
  return buf;
}

std::string stripQuotes(const std::string &s) {
  if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
    return s.substr(1, s.size() - 2);
  }
  return s;
}

class Parser {
public:
  explicit Parser(std::vector<Token> tokens) : toks(std::move(tokens)) {}

  ast::Program parse() {
    ast::Program prog;
    // main -> includes, states, uniforms, attributes, functions, post-incs
    while (at(Tok::KWInclude)) {
      next();
      prog.includes.push_back(expect(Tok::String).value); // keeps quotes
    }
    while (at(Tok::VarName)) {
      prog.states.push_back(parseStateSection());
    }
    while (at(Tok::KWUniform)) {
      prog.uniforms.push_back(parseUniform());
    }
    while (at(Tok::KWAttr)) {
      prog.attributes.push_back(parseAttribute());
    }
    while (at(Tok::FunctionType) || at(Tok::AnnoStart)) {
      prog.functions.push_back(parseFunction());
    }
    while (at(Tok::KWInclude)) {
      next();
      prog.postIncludes.push_back(expect(Tok::String).value);
    }
    expect(Tok::End);
    return prog;
  }

private:
  std::vector<Token> toks;
  size_t pos = 0;

  // --- Token helpers ---------------------------------------------------

  const Token &cur() const { return toks[pos]; }
  const Token &peekNext() const {
    return toks[pos + 1 < toks.size() ? pos + 1 : toks.size() - 1];
  }
  bool at(Tok t) const { return cur().type == t; }
  bool atAdj(Tok t) const { return cur().type == t && !cur().spaceBefore; }
  bool atNumeric() const {
    return at(Tok::ValueHex) || at(Tok::ValueBin) || at(Tok::ValueFloat) ||
           at(Tok::ValueDec);
  }
  bool atNumericAdj() const { return atNumeric() && !cur().spaceBefore; }

  Token next() { return toks[pos++]; }

  [[noreturn]] void error() const {
    const Token &t = cur();
    std::string what = (t.type == Tok::End)
                           ? "unexpected end of input"
                           : "unexpected " + std::string(tokName(t.type)) +
                                 " token '" + t.value + "'";
    throw std::runtime_error("Syntax error at line " + std::to_string(t.line) +
                             " col " + std::to_string(t.col) + ": " + what);
  }

  Token expect(Tok t) {
    if (!at(t)) error();
    return next();
  }
  // grammar wrote no `_` before this terminal -> must be adjacent
  Token expectAdj(Tok t) {
    if (!atAdj(t)) error();
    return next();
  }

  double parseNumeric() {
    if (!atNumeric()) error();
    return parseNumericToken(next());
  }
  // ValueNumeric with no preceding `_` in the grammar
  double parseNumericAdj() {
    if (!atNumericAdj()) error();
    return parseNumericToken(next());
  }

  // --- Sections --------------------------------------------------------

  // SectionState -> %VarName _ %BlockStart _ StateVarDef:* %BlockEnd
  ast::StateSection parseStateSection() {
    ast::StateSection sec;
    sec.name = expect(Tok::VarName).value;
    expect(Tok::BlockStart);
    while (!at(Tok::BlockEnd)) {
      sec.vars.push_back(parseStateVarDef());
    }
    next(); // }
    return sec;
  }

  // StateVarDef -> (%KWExtern _):? StateAlign:? %DataType _ %VarName
  //                IndexDef:* StateValueDef:? _ %StmEnd _
  ast::StateVarDef parseStateVarDef() {
    ast::StateVarDef sv;
    if (at(Tok::KWExtern)) {
      next();
      sv.isExtern = true;
    }
    // StateAlign -> %KWAlign %ArgsStart ValueNumeric %ArgsEnd _  (all adjacent)
    if (at(Tok::KWAlign)) {
      next();
      expectAdj(Tok::ArgsStart);
      sv.align = static_cast<int64_t>(parseNumericAdj());
      expectAdj(Tok::ArgsEnd);
    }
    sv.varType = expect(Tok::DataType).value;
    sv.varName = expect(Tok::VarName).value;
    // IndexDef:* — each `[` adjacent to the name / previous `]`
    while (atAdj(Tok::IdxStart)) {
      next();
      sv.arraySize.push_back(static_cast<int64_t>(parseNumeric()));
      expect(Tok::IdxEnd);
    }
    // StateValueDef -> _ %Assignment _ %BlockStart _ NumList _ %BlockEnd
    if (at(Tok::Assignment)) {
      next();
      expect(Tok::BlockStart);
      sv.value.push_back(static_cast<int64_t>(parseNumeric()));
      while (at(Tok::Seperator)) {
        next();
        sv.value.push_back(static_cast<int64_t>(parseNumeric()));
      }
      expect(Tok::BlockEnd);
    }
    expect(Tok::StmEnd); // `_ %StmEnd` — space allowed here
    return sv;
  }

  // RegNumDef -> %TypeStart ValueNumeric %TypeEnd  (all adjacent)
  int64_t parseRegNumDef() {
    next(); // < (caller checked adjacency)
    int64_t v = static_cast<int64_t>(parseNumericAdj());
    expectAdj(Tok::TypeEnd);
    return v;
  }

  // RegDef -> %TypeStart %Registers %TypeEnd  (all adjacent)
  std::string parseRegDef() {
    next(); // <
    Token reg = expectAdj(Tok::Register);
    expectAdj(Tok::TypeEnd);
    return reg.value;
  }

  // Uniform -> %KWUniform RegNumDef:? _ %VarName _ %BlockStart _
  //            StateVarDef:* %BlockEnd
  ast::Uniform parseUniform() {
    ast::Uniform u;
    next(); // uniform
    if (atAdj(Tok::TypeStart)) u.binding = parseRegNumDef();
    Token name = expect(Tok::VarName);
    u.name = name.value;
    u.line = name.line;
    expect(Tok::BlockStart);
    while (!at(Tok::BlockEnd)) {
      u.state.push_back(parseStateVarDef());
    }
    next(); // }
    return u;
  }

  // VertexAttribute -> %KWAttr RegNumDef:? _ %DataType _ %VarName
  //                    IndexDef:* %QuestionMark:? _ %StmEnd
  ast::Attribute parseAttribute() {
    ast::Attribute a;
    next(); // attribute
    if (atAdj(Tok::TypeStart)) a.binding = parseRegNumDef();
    a.type = expect(Tok::DataType).value;
    Token name = expect(Tok::VarName);
    a.name = name.value;
    a.line = name.line;
    while (atAdj(Tok::IdxStart)) {
      next();
      a.arraySize.push_back(static_cast<int64_t>(parseNumeric()));
      expect(Tok::IdxEnd);
    }
    if (atAdj(Tok::QuestionMark)) {
      next();
      a.optional = true;
    }
    expect(Tok::StmEnd);
    return a;
  }

  // --- Annotations -----------------------------------------------------

  // Annotation -> _ %AnnoStart %VarName (%ArgsStart AnnotationArg %ArgsEnd):?
  // All parts adjacent; the arg is a string or a number.
  // Returns {name, value, valueIsString, line-of-@}.
  struct AnnoResult {
    std::string name;
    std::string value;
    bool hasValue = false;
    bool valueIsString = false;
    uint32_t line = 0;
  };

  AnnoResult parseAnnotation() {
    AnnoResult res;
    Token atTok = next(); // @
    res.line = atTok.line;
    res.name = expectAdj(Tok::VarName).value;
    if (atAdj(Tok::ArgsStart)) {
      next();
      if (atAdj(Tok::String)) {
        res.value = stripQuotes(next().value);
        res.valueIsString = true;
      } else if (atNumericAdj()) {
        res.value = jsNumToString(parseNumericToken(next()));
      } else {
        error();
      }
      res.hasValue = true;
      expectAdj(Tok::ArgsEnd);
    }
    return res;
  }

  // --- Functions -------------------------------------------------------

  // Function -> Annotation:* _ %FunctionType (RegDef | RegNumDef):? _
  //             %VarName %ArgsStart _ FunctionDefArgs:* _ %ArgsEnd
  //             (ScopedBlock | %StmEnd)
  ast::Function parseFunction() {
    ast::Function fn;
    while (at(Tok::AnnoStart)) {
      AnnoResult a = parseAnnotation();
      // function-level annotations go through optStr in the JSON path:
      // numbers become int64 strings, missing values become ""
      fn.annotations.push_back(
          {a.name, a.value, a.valueIsString});
    }
    Token ftype = expect(Tok::FunctionType);
    fn.type = toFuncType(ftype.value);
    if (atAdj(Tok::TypeStart)) {
      fn.hasResultType = true;
      if (toks[pos + 1].type == Tok::Register && !toks[pos + 1].spaceBefore) {
        parseRegDef(); // register result type: kept only as hasResultType
      } else {
        fn.resultType = parseRegNumDef();
      }
    }
    fn.name = expect(Tok::VarName).value;
    expectAdj(Tok::ArgsStart);
    if (!at(Tok::ArgsEnd)) {
      fn.args.push_back(parseFuncDefArg());
      while (at(Tok::Seperator)) {
        next();
        fn.args.push_back(parseFuncDefArg());
      }
    }
    expect(Tok::ArgsEnd); // `_ %ArgsEnd` — space allowed
    if (atAdj(Tok::StmEnd)) {
      next(); // forward declaration, no body
    } else {
      fn.body = std::make_unique<ast::ScopedBlock>(parseScopedBlock());
    }
    return fn;
  }

  // FunctonDefArg -> %DataType RegDef:? _ %VarName
  ast::FuncDefArg parseFuncDefArg() {
    ast::FuncDefArg arg;
    arg.type = toTypeClass(expect(Tok::DataType).value);
    if (atAdj(Tok::TypeStart)) arg.reg = parseRegDef();
    arg.name = expect(Tok::VarName).value;
    return arg;
  }

  // --- Statements ------------------------------------------------------

  // ScopedBlock -> _ %BlockStart Statements _ %BlockEnd
  ast::ScopedBlock parseScopedBlock() {
    ast::ScopedBlock block;
    block.line = expect(Tok::BlockStart).line;
    while (!at(Tok::BlockEnd)) {
      parseStatementInto(block);
    }
    next(); // }
    return block;
  }

  // Wraps a single statement like the grammar's FORCE_SCOPED_BLOCK: the
  // synthetic block has no line of its own.
  static ast::ScopedBlock wrapInBlock(ast::Stmt stmt) {
    ast::ScopedBlock block;
    block.line = 0;
    block.statements.push_back(std::move(stmt));
    return block;
  }

  void parseStatementInto(ast::ScopedBlock &block) {
    if (at(Tok::BlockStart)) {
      ast::StmtScopedBlock sb;
      auto inner = parseScopedBlock();
      sb.line = inner.line;
      sb.body = std::make_unique<ast::ScopedBlock>(std::move(inner));
      block.statements.push_back(std::move(sb));
      return;
    }
    if (at(Tok::AnnoStart)) {
      AnnoResult a = parseAnnotation();
      ast::StmtAnnotation s;
      s.name = a.name;
      s.value = a.value;
      s.valueIsString = a.valueIsString;
      s.line = a.line;
      block.statements.push_back(std::move(s));
      return;
    }
    if (at(Tok::KWIf)) {
      block.statements.push_back(parseIf());
      return;
    }
    if (at(Tok::KWWhile)) {
      block.statements.push_back(parseWhile());
      return;
    }
    if (at(Tok::KWLoop)) {
      block.statements.push_back(parseLoop());
      return;
    }
    // Local macro definition
    if (at(Tok::FunctionType)) {
      uint32_t line = cur().line;
      if (toFuncType(cur().value) != FuncType::Macro) {
        throw std::runtime_error(
            "Syntax error at line " + std::to_string(line) +
            ": only macros can be declared inside a function ('" +
            cur().value + "' must be global)");
      }
      ast::Function fn = parseFunction();
      if (fn.hasResultType) {
        throw std::runtime_error(
            "Syntax error at line " + std::to_string(line) +
            ": Macros must not specify a result-type (use 'macro' "
            "without `< >`)!");
      }
      if (!fn.body) {
        throw std::runtime_error(
            "Syntax error at line " + std::to_string(line) +
            ": local macros must have a body (forward declarations "
            "are not allowed inside functions)");
      }
      ast::StmtMacroDef s;
      s.def = std::make_unique<ast::Function>(std::move(fn));
      s.line = line;
      block.statements.push_back(std::move(s));
      return;
    }
    // LabelDecl -> _ %VarName %Colon (colon adjacent)
    if (at(Tok::VarName) && peekNext().type == Tok::Colon &&
        !peekNext().spaceBefore) {
      Token name = next();
      next(); // :
      block.statements.push_back(ast::StmtLabelDecl{name.value, name.line});
      return;
    }
    block.statements.push_back(parseExpressionStatement());
  }

  // IfStatement -> _ %KWIf _ %ArgsStart (ExprCompare | ExprCompareBool) _
  //                %ArgsEnd (ScopedBlock | Expression)
  //                (_ %KWElse (ScopedBlock | Expression | IfStatement)):?
  ast::Stmt parseIf() {
    ast::StmtIf s;
    s.line = next().line; // if
    expect(Tok::ArgsStart);
    s.compare = parseCompareExpr(/*allowBool=*/true);
    expect(Tok::ArgsEnd);
    s.blockIf = std::make_unique<ast::ScopedBlock>(parseBlockOrExpr());
    if (at(Tok::KWElse)) {
      next();
      if (at(Tok::KWIf)) {
        s.blockElse = std::make_unique<ast::ScopedBlock>(
            wrapInBlock(parseIf()));
      } else {
        s.blockElse = std::make_unique<ast::ScopedBlock>(parseBlockOrExpr());
      }
    }
    return ast::Stmt{std::move(s)};
  }

  ast::ScopedBlock parseBlockOrExpr() {
    if (at(Tok::BlockStart)) return parseScopedBlock();
    return wrapInBlock(parseExpressionStatement());
  }

  // WhileStatement -> _ %KWWhile _ %ArgsStart ExprCompare _ %ArgsEnd
  //                   ScopedBlock  (full compare + block required)
  ast::Stmt parseWhile() {
    ast::StmtWhile s;
    s.line = next().line; // while
    expect(Tok::ArgsStart);
    s.compare = parseCompareExpr(/*allowBool=*/false);
    expect(Tok::ArgsEnd);
    s.block = std::make_unique<ast::ScopedBlock>(parseScopedBlock());
    return ast::Stmt{std::move(s)};
  }

  // LoopStatement -> _ %KWLoop ScopedBlock
  //                  (_ %KWWhile _ %ArgsStart ExprCompare _ %ArgsEnd):?
  ast::Stmt parseLoop() {
    ast::StmtLoop s;
    s.line = next().line; // loop
    s.block = std::make_unique<ast::ScopedBlock>(parseScopedBlock());
    if (at(Tok::KWWhile)) {
      next();
      expect(Tok::ArgsStart);
      s.compare = parseCompareExpr(/*allowBool=*/false);
      expect(Tok::ArgsEnd);
    }
    return ast::Stmt{std::move(s)};
  }

  // ExprCompare     -> _ FuncArg _ (%OperatorCompare|%TypeStart|%TypeEnd) _ FuncArg
  // ExprCompareBool -> _ %OperatorUnary:? FuncArg
  // Neither form records a line (the grammar reads .line off a FuncArg,
  // which has none) — so CompareExpr.line is always 0, like the JSON path.
  ast::CompareExpr parseCompareExpr(bool allowBool) {
    ast::CompareExpr cmp;
    cmp.line = 0;
    if (allowBool && at(Tok::OperatorUnary)) {
      Token op = next();
      // unary adjacent to the FuncArg
      if (cur().spaceBefore) error();
      cmp.left = parseFuncArg();
      cmp.op = (op.value == "!") ? "==" : "!=";
      cmp.right = ast::FuncArg{ArgType::Num, "0", ""};
      return cmp;
    }
    cmp.left = parseFuncArg();
    if (at(Tok::OperatorCompare) || at(Tok::TypeStart) || at(Tok::TypeEnd)) {
      cmp.op = next().value;
      cmp.right = parseFuncArg();
      return cmp;
    }
    if (!allowBool) error();
    cmp.op = "!=";
    cmp.right = ast::FuncArg{ArgType::Num, "0", ""};
    return cmp;
  }

  // FuncArg -> %VarName %Swizzle:? | ValueNumeric | %String
  ast::FuncArg parseFuncArg() {
    if (at(Tok::VarName)) {
      Token name = next();
      std::string sw;
      if (atAdj(Tok::Swizzle)) sw = next().value;
      return ast::FuncArg{ArgType::Var, name.value, sw};
    }
    if (atNumeric()) {
      // the JSON path renders numeric args through int64 (jsonAsStr)
      return ast::FuncArg{
          ArgType::Num,
          std::to_string(static_cast<int64_t>(parseNumeric())), ""};
    }
    if (at(Tok::String)) {
      return ast::FuncArg{ArgType::String, stripQuotes(next().value), ""};
    }
    error();
  }

  // Expression -> _ (...alternatives...) %StmEnd   (`;` adjacent!)
  ast::Stmt parseExpressionStatement() {
    ast::Stmt stmt = parseExpression();
    expectAdj(Tok::StmEnd);
    return stmt;
  }

  ast::Stmt parseExpression() {
    // ExprVarDeclAssign / ExprVarDecl
    if (at(Tok::KWConst) || at(Tok::DataType)) {
      bool isConst = false;
      if (at(Tok::KWConst)) {
        next();
        isConst = true;
        if (!cur().spaceBefore) error(); // (%KWConst __) — space required
      }
      Token dt = expect(Tok::DataType);
      std::string reg;
      if (atAdj(Tok::TypeStart)) reg = parseRegDef(); // register only
      Token firstName = expect(Tok::VarName);
      if (at(Tok::Assignment)) {
        next();
        ast::StmtVarDeclAssign s;
        s.varType = dt.value;
        s.reg = reg;
        s.varName = firstName.value;
        s.isConst = isConst;
        s.line = dt.line;
        s.calc = std::make_unique<ast::Calc>(parseCalcAll());
        return ast::Stmt{std::move(s)};
      }
      ast::StmtVarDeclMulti s;
      s.varType = dt.value;
      s.reg = reg;
      s.isConst = isConst;
      s.line = dt.line;
      s.varNames.push_back(firstName.value);
      while (at(Tok::Seperator)) {
        next();
        s.varNames.push_back(expect(Tok::VarName).value);
      }
      return ast::Stmt{std::move(s)};
    }
    if (at(Tok::KWUndef)) {
      Token kw = next();
      return ast::Stmt{
          ast::StmtVarUndef{expect(Tok::VarName).value, kw.line}};
    }
    if (at(Tok::KWGoto)) {
      next();
      Token label = expect(Tok::VarName); // line comes from the label token
      return ast::Stmt{ast::StmtGoto{label.value, label.line}};
    }
    if (at(Tok::KWContinue)) return ast::Stmt{ast::StmtContinue{next().line}};
    if (at(Tok::KWBreak)) return ast::Stmt{ast::StmtBreak{next().line}};
    if (at(Tok::KWExit)) return ast::Stmt{ast::StmtExit{next().line}};

    if (at(Tok::VarName)) {
      // ExprFuncCall -> %VarName %ArgsStart _ FuncArgs:* %ArgsEnd
      if (peekNext().type == Tok::ArgsStart && !peekNext().spaceBefore) {
        Token name = next();
        next(); // (
        ast::StmtFuncCall s;
        s.func = name.value;
        s.line = name.line;
        if (!at(Tok::ArgsEnd)) {
          s.args.push_back(parseFuncArg());
          while (at(Tok::Seperator)) {
            next();
            s.args.push_back(parseFuncArg());
          }
          expectAdj(Tok::ArgsEnd); // no `_` before `)` at statement level
        } else {
          next(); // )
        }
        return ast::Stmt{std::move(s)};
      }
      // ExprVarAssign -> %VarName %Swizzle:? _ (%Assignment|%OperatorSelfR)
      //                  _ ExprCalcAll
      Token name = next();
      ast::StmtVarAssignCalc s;
      s.varName = name.value;
      s.line = name.line;
      if (atAdj(Tok::Swizzle)) s.swizzle = next().value;
      if (at(Tok::Assignment) || at(Tok::OperatorSelfR)) {
        s.assignType = next().value;
      } else {
        error();
      }
      s.calc = std::make_unique<ast::Calc>(parseCalcAll());
      return ast::Stmt{std::move(s)};
    }
    error();
  }

  // --- Calculations ----------------------------------------------------

  // ExprCalcAll -> ExprCalcMulti | ExprCalcNum | ExprCalcVar |
  //                ExprCalcFunc | ExprCalcCompare
  ast::Calc parseCalcAll() {
    if (at(Tok::ArgsStart)) {
      return parseCalcMulti();
    }
    if (atNumeric()) {
      Token numTok = next();
      double v = parseNumericToken(numTok);
      if (at(Tok::OperatorLR)) {
        return parseCalcMultiFromNum(v);
      }
      return ast::Calc{ast::CalcNum{ast::ExprNum{v}}};
    }
    // ExprCalcVar -> %OperatorUnary:? %VarName %Swizzle:?  (all adjacent)
    if (at(Tok::OperatorUnary)) {
      Token op = next();
      Token name = expectAdj(Tok::VarName);
      std::string sw;
      if (atAdj(Tok::Swizzle)) sw = next().value;
      return ast::Calc{
          ast::CalcVar{op.value, ast::ExprVarName{name.value}, sw}};
    }
    if (at(Tok::VarName)) {
      // ExprCalcFunc -> %VarName %ArgsStart _ FuncArgs:* _ %ArgsEnd %Swizzle:?
      if (peekNext().type == Tok::ArgsStart && !peekNext().spaceBefore) {
        Token name = next();
        next(); // (
        ast::CalcFunc cf;
        cf.funcName = name.value;
        if (!at(Tok::ArgsEnd)) {
          cf.args.push_back(parseFuncArg());
          while (at(Tok::Seperator)) {
            next();
            cf.args.push_back(parseFuncArg());
          }
        }
        expect(Tok::ArgsEnd); // `_ %ArgsEnd` — space allowed (unlike stmt call)
        if (atAdj(Tok::Swizzle)) cf.swizzleRight = next().value;
        return ast::Calc{std::move(cf)};
      }
      Token name = next();
      std::string sw;
      if (atAdj(Tok::Swizzle)) sw = next().value;
      if (at(Tok::OperatorLR)) {
        return parseCalcMultiFromVar(name.value, sw);
      }
      if (sw.empty() && (at(Tok::OperatorCompare) || at(Tok::TypeStart) ||
                         at(Tok::TypeEnd))) {
        return parseCalcCompare(name.value);
      }
      return ast::Calc{ast::CalcVar{"", ast::ExprVarName{name.value}, sw}};
    }
    error();
  }

  // ExprCalcMulti -> (%ArgsStart _):* (ExprVarName|ExprNum) %Swizzle:?
  //                  (_ ExprCalcMultiPart):+
  ast::Calc parseCalcMulti() {
    ast::CalcMulti cm;
    while (at(Tok::ArgsStart)) {
      next();
      ++cm.groupStart;
    }
    if (at(Tok::VarName)) {
      cm.left = ast::ExprVarName{next().value};
    } else {
      // the JSON path reads calcMulti operand numbers through int64
      cm.leftVal = static_cast<double>(static_cast<int64_t>(parseNumeric()));
    }
    if (atAdj(Tok::Swizzle)) cm.swizzleLeft = next().value;
    parseCalcMultiParts(cm);
    return ast::Calc{std::move(cm)};
  }

  ast::Calc parseCalcMultiFromVar(std::string name, std::string swizzle) {
    ast::CalcMulti cm;
    cm.left = ast::ExprVarName{std::move(name)};
    cm.swizzleLeft = std::move(swizzle);
    parseCalcMultiParts(cm);
    return ast::Calc{std::move(cm)};
  }

  ast::Calc parseCalcMultiFromNum(double v) {
    ast::CalcMulti cm;
    cm.leftVal = static_cast<double>(static_cast<int64_t>(v));
    parseCalcMultiParts(cm);
    return ast::Calc{std::move(cm)};
  }

  // ExprCalcMultiPart -> %OperatorLR _ (%ArgsStart _):* (ExprVarName|ExprNum)
  //                      %Swizzle:? (_ %ArgsEnd):*
  void parseCalcMultiParts(ast::CalcMulti &cm) {
    if (!at(Tok::OperatorLR)) error(); // at least one part
    while (at(Tok::OperatorLR)) {
      ast::CalcMultiPart part;
      part.op = next().value;
      while (at(Tok::ArgsStart)) {
        next();
        ++part.groupStart;
      }
      if (at(Tok::VarName)) {
        part.right = ast::ExprVarName{next().value};
      } else {
        part.rightVal =
            static_cast<double>(static_cast<int64_t>(parseNumeric()));
      }
      if (atAdj(Tok::Swizzle)) part.swizzleRight = next().value;
      while (at(Tok::ArgsEnd)) {
        next();
        ++part.groupEnd;
      }
      cm.parts.push_back(std::move(part));
    }
  }

  // ExprCalcCompare -> %VarName _ (op) _ (%VarName | ValueNumeric)
  //                    %Swizzle:? ExprPartTernary:?
  ast::Calc parseCalcCompare(std::string left) {
    ast::CalcCompare cc;
    cc.left = std::move(left);
    cc.op = next().value; // compare op / < / >
    if (at(Tok::VarName)) {
      cc.right = next().value;
    } else {
      cc.rightVal = parseNumeric(); // kept as double in the JSON path
    }
    if (atAdj(Tok::Swizzle)) cc.swizzleRight = next().value;
    // ExprPartTernary -> _ %QuestionMark _ %VarName _ %Colon _
    //                    (%VarName | ValueNumeric) %Swizzle:? _
    if (at(Tok::QuestionMark)) {
      next();
      ast::TernaryPart tp;
      tp.left = expect(Tok::VarName).value;
      expect(Tok::Colon);
      if (at(Tok::VarName)) {
        tp.right = next().value;
      } else {
        tp.rightVal = parseNumeric();
      }
      if (atAdj(Tok::Swizzle)) tp.swizzleRight = next().value;
      cc.ternary = std::move(tp);
    }
    return ast::Calc{std::move(cc)};
  }
};

} // namespace

ast::Program parseProgram(const std::string &source) {
  return Parser(tokenize(source)).parse();
}

} // namespace rspl::parser

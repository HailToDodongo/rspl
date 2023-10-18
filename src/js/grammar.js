// Generated automatically by nearley, version 2.20.1
// http://github.com/Hardmath123/nearley
(function () {
function id(x) { return x[0]; }
 
	const MAP_NULL = () => null;
	const MAP_FIRST = d => d[0]; 
var grammar = {
    Lexer: undefined,
    ParserRules: [
    {"name": "Function$string$1", "symbols": [{"literal":"("}, {"literal":")"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "Function", "symbols": ["__", "FuncName", "Function$string$1", "_LB_", {"literal":"{"}, "FuncBody", "_LB_", {"literal":"}"}], "postprocess": (d) => ({type: "function", name: d[1], body: d[5]})},
    {"name": "FuncBody$ebnf$1", "symbols": []},
    {"name": "FuncBody$ebnf$1$subexpression$1", "symbols": ["_", "Statement", "_", {"literal":";"}]},
    {"name": "FuncBody$ebnf$1", "symbols": ["FuncBody$ebnf$1", "FuncBody$ebnf$1$subexpression$1"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "FuncBody", "symbols": ["FuncBody$ebnf$1"], "postprocess": function(d) {return {type: "funcBody", statements: d[0].map(y => y[1])}}},
    {"name": "Statement$subexpression$1", "symbols": ["ExprVarDeclAssign"]},
    {"name": "Statement$subexpression$1", "symbols": ["ExprVarAssign"]},
    {"name": "Statement$subexpression$1", "symbols": ["ExprASM"]},
    {"name": "Statement", "symbols": ["Statement$subexpression$1"], "postprocess": (d) => d[0][0]},
    {"name": "ExprVarDeclAssign$subexpression$1$ebnf$1", "symbols": ["ExprPartAssign"], "postprocess": id},
    {"name": "ExprVarDeclAssign$subexpression$1$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprVarDeclAssign$subexpression$1", "symbols": ["DataType", "RegDef", "__", "VarName", "__", "ExprVarDeclAssign$subexpression$1$ebnf$1"]},
    {"name": "ExprVarDeclAssign", "symbols": ["ExprVarDeclAssign$subexpression$1"], "postprocess":  
        ([d]) => ({type: "varDeclAssign", varType: d[0], reg: d[1], varName: d[3], value: d[5]})
        },
    {"name": "ExprPartAssign$subexpression$1", "symbols": ["ValueNumeric"]},
    {"name": "ExprPartAssign$subexpression$1", "symbols": ["ExprFuncCall"]},
    {"name": "ExprPartAssign$ebnf$1", "symbols": ["OpsSwizzle"], "postprocess": id},
    {"name": "ExprPartAssign$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprPartAssign", "symbols": [{"literal":"="}, "__", "ExprPartAssign$subexpression$1", "__", "ExprPartAssign$ebnf$1"], "postprocess": d => ({type: "value", value: d[2][0], swizzle: d[4]})},
    {"name": "ExprFuncCall", "symbols": ["FuncName", {"literal":"("}, "__", "VarName", "__", {"literal":")"}], "postprocess": d => ({type: "funcCall", func: d[0], args: d[3]})},
    {"name": "ExprASM$string$1", "symbols": [{"literal":"a"}, {"literal":"s"}, {"literal":"m"}, {"literal":"("}, {"literal":"\""}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "ExprASM$ebnf$1", "symbols": []},
    {"name": "ExprASM$ebnf$1", "symbols": ["ExprASM$ebnf$1", /[a-zA-Z0-9 ]/], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "ExprASM$string$2", "symbols": [{"literal":"\""}, {"literal":")"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "ExprASM", "symbols": ["ExprASM$string$1", "ExprASM$ebnf$1", "ExprASM$string$2"], "postprocess": d => ({type: "asm", asm: d[1].join("")})},
    {"name": "ExprVarAssign$subexpression$1", "symbols": ["VarName", "__", {"literal":"="}, "__", "ExprPartAssignCalc"]},
    {"name": "ExprVarAssign", "symbols": ["ExprVarAssign$subexpression$1"], "postprocess":  
        ([d]) => ({type: "varAssignCalc", varName: d[0], value: d[5], calc: d[4]})
        },
    {"name": "ExprPartAssignCalc$ebnf$1", "symbols": ["OpsSwizzle"], "postprocess": id},
    {"name": "ExprPartAssignCalc$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprPartAssignCalc", "symbols": ["VarName", "__", "OpsNumeric", "__", "VarName", "ExprPartAssignCalc$ebnf$1"]},
    {"name": "VarName$ebnf$1", "symbols": [/[a-zA-Z0-9_]/]},
    {"name": "VarName$ebnf$1", "symbols": ["VarName$ebnf$1", /[a-zA-Z0-9_]/], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "VarName", "symbols": ["VarName$ebnf$1"], "postprocess": d => d[0].join("")},
    {"name": "FuncName$ebnf$1", "symbols": [/[a-zA-Z0-9_]/]},
    {"name": "FuncName$ebnf$1", "symbols": ["FuncName$ebnf$1", /[a-zA-Z0-9_]/], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "FuncName", "symbols": ["FuncName$ebnf$1"], "postprocess": d => d[0].join("")},
    {"name": "DataType$subexpression$1$string$1", "symbols": [{"literal":"u"}, {"literal":"3"}, {"literal":"2"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "DataType$subexpression$1", "symbols": ["DataType$subexpression$1$string$1"]},
    {"name": "DataType$subexpression$1$string$2", "symbols": [{"literal":"s"}, {"literal":"3"}, {"literal":"2"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "DataType$subexpression$1", "symbols": ["DataType$subexpression$1$string$2"]},
    {"name": "DataType$subexpression$1$string$3", "symbols": [{"literal":"v"}, {"literal":"e"}, {"literal":"c"}, {"literal":"3"}, {"literal":"2"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "DataType$subexpression$1", "symbols": ["DataType$subexpression$1$string$3"]},
    {"name": "DataType$subexpression$1$string$4", "symbols": [{"literal":"v"}, {"literal":"e"}, {"literal":"c"}, {"literal":"1"}, {"literal":"6"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "DataType$subexpression$1", "symbols": ["DataType$subexpression$1$string$4"]},
    {"name": "DataType", "symbols": ["DataType$subexpression$1"], "postprocess": (d) => d[0][0]},
    {"name": "RegsAll", "symbols": ["RegsScalar"]},
    {"name": "RegsAll", "symbols": ["RegsVector"]},
    {"name": "RegsScalar$subexpression$1$string$1", "symbols": [{"literal":"$"}, {"literal":"t"}, {"literal":"0"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "RegsScalar$subexpression$1", "symbols": ["RegsScalar$subexpression$1$string$1"]},
    {"name": "RegsScalar$subexpression$1$string$2", "symbols": [{"literal":"$"}, {"literal":"t"}, {"literal":"1"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "RegsScalar$subexpression$1", "symbols": ["RegsScalar$subexpression$1$string$2"]},
    {"name": "RegsScalar", "symbols": ["RegsScalar$subexpression$1"], "postprocess": (d) => d[0][0]},
    {"name": "RegsVector$subexpression$1$string$1", "symbols": [{"literal":"$"}, {"literal":"v"}, {"literal":"0"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "RegsVector$subexpression$1", "symbols": ["RegsVector$subexpression$1$string$1"]},
    {"name": "RegsVector$subexpression$1$string$2", "symbols": [{"literal":"$"}, {"literal":"v"}, {"literal":"1"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "RegsVector$subexpression$1", "symbols": ["RegsVector$subexpression$1$string$2"]},
    {"name": "RegsVector", "symbols": ["RegsVector$subexpression$1"], "postprocess": (d) => d[0][0]},
    {"name": "RegDef", "symbols": [{"literal":"<"}, "RegsAll", {"literal":">"}], "postprocess": (d) => d[1][0]},
    {"name": "ValueNumeric$subexpression$1", "symbols": ["ValueInt"]},
    {"name": "ValueNumeric$subexpression$1", "symbols": ["ValueHex"]},
    {"name": "ValueNumeric", "symbols": ["ValueNumeric$subexpression$1"], "postprocess": (d) => d[0][0]},
    {"name": "ValueInt$ebnf$1", "symbols": [/[0-9]/]},
    {"name": "ValueInt$ebnf$1", "symbols": ["ValueInt$ebnf$1", /[0-9]/], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "ValueInt", "symbols": ["ValueInt$ebnf$1"], "postprocess": (d) => parseInt(d[0].join(""))},
    {"name": "ValueHex$string$1", "symbols": [{"literal":"0"}, {"literal":"x"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "ValueHex$ebnf$1", "symbols": [/[0-9A-F]/]},
    {"name": "ValueHex$ebnf$1", "symbols": ["ValueHex$ebnf$1", /[0-9A-F]/], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "ValueHex", "symbols": ["ValueHex$string$1", "ValueHex$ebnf$1"], "postprocess": (d) => parseInt(d[1].join(""), 16)},
    {"name": "OpsNumeric$subexpression$1", "symbols": [{"literal":"+"}]},
    {"name": "OpsNumeric$subexpression$1", "symbols": [{"literal":"-"}]},
    {"name": "OpsNumeric$subexpression$1", "symbols": [{"literal":"*"}]},
    {"name": "OpsNumeric$subexpression$1$string$1", "symbols": [{"literal":"+"}, {"literal":"*"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "OpsNumeric$subexpression$1", "symbols": ["OpsNumeric$subexpression$1$string$1"]},
    {"name": "OpsNumeric$subexpression$1", "symbols": [{"literal":"/"}]},
    {"name": "OpsNumeric$subexpression$1", "symbols": [{"literal":"."}]},
    {"name": "OpsNumeric", "symbols": ["OpsNumeric$subexpression$1"]},
    {"name": "OpsSwizzle$subexpression$1$string$1", "symbols": [{"literal":"."}, {"literal":"x"}, {"literal":"x"}, {"literal":"x"}, {"literal":"x"}, {"literal":"X"}, {"literal":"X"}, {"literal":"X"}, {"literal":"X"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "OpsSwizzle$subexpression$1", "symbols": ["OpsSwizzle$subexpression$1$string$1"]},
    {"name": "OpsSwizzle$subexpression$1$string$2", "symbols": [{"literal":"."}, {"literal":"y"}, {"literal":"y"}, {"literal":"y"}, {"literal":"y"}, {"literal":"Y"}, {"literal":"Y"}, {"literal":"Y"}, {"literal":"Y"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "OpsSwizzle$subexpression$1", "symbols": ["OpsSwizzle$subexpression$1$string$2"]},
    {"name": "OpsSwizzle$subexpression$1$string$3", "symbols": [{"literal":"."}, {"literal":"z"}, {"literal":"z"}, {"literal":"z"}, {"literal":"z"}, {"literal":"Z"}, {"literal":"Z"}, {"literal":"Z"}, {"literal":"Z"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "OpsSwizzle$subexpression$1", "symbols": ["OpsSwizzle$subexpression$1$string$3"]},
    {"name": "OpsSwizzle$subexpression$1$string$4", "symbols": [{"literal":"."}, {"literal":"w"}, {"literal":"w"}, {"literal":"w"}, {"literal":"w"}, {"literal":"W"}, {"literal":"W"}, {"literal":"W"}, {"literal":"W"}], "postprocess": function joiner(d) {return d.join('');}},
    {"name": "OpsSwizzle$subexpression$1", "symbols": ["OpsSwizzle$subexpression$1$string$4"]},
    {"name": "OpsSwizzle", "symbols": ["OpsSwizzle$subexpression$1"], "postprocess": (d) => d[0][0]},
    {"name": "_$ebnf$1", "symbols": []},
    {"name": "_$ebnf$1", "symbols": ["_$ebnf$1", /[\s]/], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "_", "symbols": ["_$ebnf$1"], "postprocess": MAP_NULL},
    {"name": "__$ebnf$1", "symbols": []},
    {"name": "__$ebnf$1", "symbols": ["__$ebnf$1", /[ +]/], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "__", "symbols": ["__$ebnf$1"], "postprocess": MAP_NULL},
    {"name": "_LB_$ebnf$1", "symbols": []},
    {"name": "_LB_$ebnf$1", "symbols": ["_LB_$ebnf$1", /[ \t\n]/], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "_LB_", "symbols": ["_LB_$ebnf$1"], "postprocess": MAP_NULL}
]
  , ParserStart: "Function"
}
if (typeof module !== 'undefined'&& typeof module.exports !== 'undefined') {
   module.exports = grammar;
} else {
   window.grammar = grammar;
}
})();

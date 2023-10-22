// Generated automatically by nearley, version 2.20.1
// http://github.com/Hardmath123/nearley
(function () {
function id(x) { return x[0]; }

const MAP_NULL = () => null;
const MAP_FIRST = d => d[0];
const MAP_TAKE = (d, i) => d && d.map(x => x[i]);
const FORCE_ARRAY = d => Array.isArray(d) ? d : (d ? [d] : []);
const SAFE_VAL = d => d && d.value;
const MAP_FLATTEN_TREE = (d, idxLeft, idxRight) => [
		...(Array.isArray(d[idxLeft]) ? d[idxLeft] : [d[idxLeft]]),
		...(Array.isArray(d[idxRight]) ? d[idxRight] : [d[idxRight]])
	];

const moo = require("moo")
const lexer = moo.compile({
	LineComment: /\/\/.*?$/,
	String: /".*"/,

	DataType: ["u8", "s8", "u16", "s16", "u32", "s32", "vec32", "vec16"],
	Registers: [
		"$at", "$zero", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
		"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
		"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
		"$k0", "$k1", "$gp", "$sp", "$fp", "$ra",

		"$v00", "$v01", "$v02", "$v03", "$v04", "$v05", "$v06", "$v07",
		"$v08", "$v09", "$v10", "$v11", "$v12", "$v13", "$v14", "$v15",
		"$v16", "$v17", "$v18", "$v19", "$v20", "$v21", "$v22", "$v23",
		"$v24", "$v25", "$v26", "$v27", "$v28", "$v29", "$v30", "$v31",
	],

	Swizzle: {match: [
	  ".xxzzXXZZ", ".yywwYYWW",
	  ".xxxxXXXX", ".yyyyYYYY", ".zzzzZZZZ", ".wwwwWWWW",
	  ".xxxxxxxx", ".yyyyyyyy", ".zzzzzzzz", ".wwwwwwww",
	  ".XXXXXXXX", ".YYYYYYYY", ".ZZZZZZZZ", ".WWWWWWWW",
	  ".xyzwxyzw",
	  ".x", ".y", ".z", ".w", ".X", ".Y", ".Z", ".W",
	], value: s => s.substr(1)},


	OperatorSelfR: [
		"&&=", "||=",
		"&=", "|=",
		"<<=", ">>=",
		"<=", ">=",
		"+*=",
		"+=", "-=", "*=", "/=",
	],

	OperatorLR: [
		"&&", "||", "==", "!=",
		"<<", ">>",
		"+*",
		"+", "-", "*", "/",
		"&", "|", "^",
	],
	OperatorUnary: [
		"!", "~",
	],

	BlockStart: "{",
	BlockEnd  : "}",
	ArgsStart : "(",
	ArgsEnd   : ")",
	TypeStart : "<",
	TypeEnd   : ">",
	StmEnd    : ";",
	Seperator : ",",
	IdxStart  : "[",
	IdxEnd    : "]",
	Colon     : ":",

	Assignment: "=",

	FunctionType: ["function", "command"],
	KWState   : "state",
	KWGoto    : "goto",
	KWInclude : "include",

	ValueHex: /0x[0-9A-F]+/,
	ValueBin: /0b[0-1]+/,
	ValueDec: /[0-9]+/,
	VarName: /[a-zA-Z0-9_]+/,

	_:  { match: /[ \t\n]+/, lineBreaks: true },

});
var grammar = {
    Lexer: lexer,
    ParserRules: [
    {"name": "main$ebnf$1", "symbols": []},
    {"name": "main$ebnf$1$subexpression$1", "symbols": ["_", "SectionIncl"]},
    {"name": "main$ebnf$1", "symbols": ["main$ebnf$1", "main$ebnf$1$subexpression$1"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "main$ebnf$2", "symbols": ["SectionState"], "postprocess": id},
    {"name": "main$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "main$ebnf$3", "symbols": []},
    {"name": "main$ebnf$3$subexpression$1", "symbols": ["_", "Function"]},
    {"name": "main$ebnf$3", "symbols": ["main$ebnf$3", "main$ebnf$3$subexpression$1"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "main", "symbols": ["main$ebnf$1", "_", "main$ebnf$2", "main$ebnf$3", "_"], "postprocess":  d => ({
        	includes: MAP_TAKE(d[0], 1),
        	state: d[2],
        	functions: MAP_TAKE(d[3], 1),
        }) },
    {"name": "SectionIncl", "symbols": [(lexer.has("KWInclude") ? {type: "KWInclude"} : KWInclude), "_", (lexer.has("String") ? {type: "String"} : String)], "postprocess": d => d[2].value},
    {"name": "SectionState$ebnf$1", "symbols": []},
    {"name": "SectionState$ebnf$1", "symbols": ["SectionState$ebnf$1", "StateVarDef"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "SectionState", "symbols": [(lexer.has("KWState") ? {type: "KWState"} : KWState), "_", (lexer.has("BlockStart") ? {type: "BlockStart"} : BlockStart), "_", "SectionState$ebnf$1", (lexer.has("BlockEnd") ? {type: "BlockEnd"} : BlockEnd)], "postprocess": d => d[4]},
    {"name": "StateVarDef$ebnf$1", "symbols": ["IndexDef"], "postprocess": id},
    {"name": "StateVarDef$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "StateVarDef", "symbols": [(lexer.has("DataType") ? {type: "DataType"} : DataType), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), "StateVarDef$ebnf$1", (lexer.has("StmEnd") ? {type: "StmEnd"} : StmEnd), "_"], "postprocess": 
        (d) => ({type: "varState", varType: d[0].value, varName: d[2].value, arraySize: d[3] || 1})
        },
    {"name": "Function$ebnf$1$subexpression$1", "symbols": ["RegDef"]},
    {"name": "Function$ebnf$1$subexpression$1", "symbols": ["RegNumDef"]},
    {"name": "Function$ebnf$1", "symbols": ["Function$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "Function$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "Function$ebnf$2", "symbols": []},
    {"name": "Function$ebnf$2", "symbols": ["Function$ebnf$2", "FunctionDefArgs"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "Function", "symbols": [(lexer.has("FunctionType") ? {type: "FunctionType"} : FunctionType), "Function$ebnf$1", "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "_", "Function$ebnf$2", "_", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd), "_", (lexer.has("BlockStart") ? {type: "BlockStart"} : BlockStart), "FuncBody", "_", (lexer.has("BlockEnd") ? {type: "BlockEnd"} : BlockEnd)], "postprocess": 
        d => ({
        	type: d[0].value,
        	resultType: d[1] && d[1][0],
        	name: d[3].value,
        	args: FORCE_ARRAY(d[6][0]),
        	body: d[11]
        })
        },
    {"name": "FuncBody$ebnf$1", "symbols": []},
    {"name": "FuncBody$ebnf$1$subexpression$1", "symbols": ["LineComment"]},
    {"name": "FuncBody$ebnf$1$subexpression$1", "symbols": ["LabelDecl"]},
    {"name": "FuncBody$ebnf$1$subexpression$1", "symbols": ["Expression"]},
    {"name": "FuncBody$ebnf$1", "symbols": ["FuncBody$ebnf$1", "FuncBody$ebnf$1$subexpression$1"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "FuncBody", "symbols": ["FuncBody$ebnf$1"], "postprocess": function(d) {return {type: "funcBody", statements: d[0].map(y => y[0])}}},
    {"name": "FunctionDefArgs", "symbols": ["FunctonDefArg"], "postprocess": MAP_FIRST},
    {"name": "FunctionDefArgs$subexpression$1", "symbols": ["FunctionDefArgs", "_", (lexer.has("Seperator") ? {type: "Seperator"} : Seperator), "_", "FunctonDefArg"]},
    {"name": "FunctionDefArgs", "symbols": ["FunctionDefArgs$subexpression$1"], "postprocess": d => MAP_FLATTEN_TREE(d[0], 0, 4)},
    {"name": "FunctonDefArg", "symbols": [(lexer.has("DataType") ? {type: "DataType"} : DataType), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName)], "postprocess": d => ({type: d[0].value, name: d[2] && d[2].value})},
    {"name": "Expression$subexpression$1", "symbols": ["ExprVarDeclAssign"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprVarDecl"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprVarAssign"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprFuncCall"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprGoto"]},
    {"name": "Expression", "symbols": ["_", "Expression$subexpression$1", (lexer.has("StmEnd") ? {type: "StmEnd"} : StmEnd)], "postprocess": (d) => d[1][0]},
    {"name": "LabelDecl", "symbols": ["_", (lexer.has("VarName") ? {type: "VarName"} : VarName), (lexer.has("Colon") ? {type: "Colon"} : Colon)], "postprocess": d => ({type: "labelDecl", name: d[1].value, line: d[1].line})},
    {"name": "LineComment", "symbols": ["_", (lexer.has("LineComment") ? {type: "LineComment"} : LineComment), /[\n]/], "postprocess": (d) => ({type: "comment", comment: d[1].value, line: d[1].line})},
    {"name": "ExprVarDeclAssign$subexpression$1", "symbols": [(lexer.has("DataType") ? {type: "DataType"} : DataType), "RegDef", "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), "_", "ExprPartAssign"]},
    {"name": "ExprVarDeclAssign", "symbols": ["ExprVarDeclAssign$subexpression$1"], "postprocess":  d => ({
        	type: "varDeclAssign", varType: d[0][0].value,
        	reg: d[0][1], varName: d[0][3].value,
        	calc: d[0][5],
        	line: d[0][0].line
        })},
    {"name": "ExprPartAssign", "symbols": [(lexer.has("Assignment") ? {type: "Assignment"} : Assignment), "_", "ExprCalcAll"], "postprocess": d => d[2][0]},
    {"name": "ExprVarDecl$subexpression$1", "symbols": [(lexer.has("DataType") ? {type: "DataType"} : DataType), "RegDef", "_", "VarList"]},
    {"name": "ExprVarDecl", "symbols": ["ExprVarDecl$subexpression$1"], "postprocess":  d => ({
        	type: "varDeclMulti",
        	varType: d[0][0].value,
        	reg: d[0][1],
        	varNames: FORCE_ARRAY(d[0][3]).map(x => x.value),
        	line: d[0][0].line
        })},
    {"name": "ExprFuncCall$subexpression$1", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName)]},
    {"name": "ExprFuncCall$subexpression$1", "symbols": [(lexer.has("String") ? {type: "String"} : String)]},
    {"name": "ExprFuncCall", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "_", "ExprFuncCall$subexpression$1", "_", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd)], "postprocess":  d => ({
        	type: "funcCall",
        	func: d[0].value, args: d[3][0],
        	line: d[0].line
        })},
    {"name": "ExprGoto", "symbols": [(lexer.has("KWGoto") ? {type: "KWGoto"} : KWGoto), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName)], "postprocess":  d => ({
        	type: "goto",
        	label: d[2].value,
        	line: d[2].line
        })},
    {"name": "ExprVarAssign$subexpression$1$ebnf$1", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprVarAssign$subexpression$1$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprVarAssign$subexpression$1$subexpression$1", "symbols": [(lexer.has("Assignment") ? {type: "Assignment"} : Assignment)]},
    {"name": "ExprVarAssign$subexpression$1$subexpression$1", "symbols": [(lexer.has("OperatorSelfR") ? {type: "OperatorSelfR"} : OperatorSelfR)]},
    {"name": "ExprVarAssign$subexpression$1", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), "ExprVarAssign$subexpression$1$ebnf$1", "_", "ExprVarAssign$subexpression$1$subexpression$1", "_", "ExprCalcAll"]},
    {"name": "ExprVarAssign", "symbols": ["ExprVarAssign$subexpression$1"], "postprocess":  ([d]) => ({
        	type: "varAssignCalc",
        	varName: d[0].value,
        	swizzle: SAFE_VAL(d[1]),
        	assignType: d[3][0].value,
        	calc: d[5][0],
        	line: d[0].line
        })},
    {"name": "ExprCalcAll", "symbols": ["ExprCalcVarVar"]},
    {"name": "ExprCalcAll", "symbols": ["ExprCalcVarNum"]},
    {"name": "ExprCalcAll", "symbols": ["ExprCalcNum"]},
    {"name": "ExprCalcAll", "symbols": ["ExprCalcVar"]},
    {"name": "ExprCalcAll", "symbols": ["ExprCalcFunc"]},
    {"name": "ExprCalcNum", "symbols": ["ValueNumeric"], "postprocess": d => ({type: "calcNum", right: d[0][0]})},
    {"name": "ExprCalcVar$ebnf$1", "symbols": [(lexer.has("OperatorUnary") ? {type: "OperatorUnary"} : OperatorUnary)], "postprocess": id},
    {"name": "ExprCalcVar$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcVar$ebnf$2", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprCalcVar$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcVar", "symbols": ["ExprCalcVar$ebnf$1", (lexer.has("VarName") ? {type: "VarName"} : VarName), "ExprCalcVar$ebnf$2"], "postprocess":  d => ({
        	type: "calcVar",
        	op: SAFE_VAL(d[0]),
        	right: d[1].value, swizzleRight: SAFE_VAL(d[2])
        })},
    {"name": "ExprCalcVarVar$ebnf$1", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprCalcVarVar$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcVarVar$ebnf$2", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprCalcVarVar$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcVarVar", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), "ExprCalcVarVar$ebnf$1", "_", "OperatorLR", "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), "ExprCalcVarVar$ebnf$2"], "postprocess":  d => ({
        	type: "calcVarVar",
        	left: d[0].value, swizzleLeft: SAFE_VAL(d[1]),
        	op: d[3].value,
        	right: d[5].value, swizzleRight: SAFE_VAL(d[6])
        })},
    {"name": "ExprCalcVarNum$ebnf$1", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprCalcVarNum$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcVarNum", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), "ExprCalcVarNum$ebnf$1", "_", "OperatorLR", "_", "ValueNumeric"], "postprocess":  d => ({
        	type: "calcVarNum",
        	left: d[0].value, swizzleLeft: SAFE_VAL(d[1]),
        	op: d[3].value,
        	right: d[5][0]
        })},
    {"name": "ExprCalcFunc$ebnf$1", "symbols": []},
    {"name": "ExprCalcFunc$ebnf$1", "symbols": ["ExprCalcFunc$ebnf$1", "FuncArgs"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "ExprCalcFunc$ebnf$2", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprCalcFunc$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcFunc", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "_", "ExprCalcFunc$ebnf$1", "_", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd), "ExprCalcFunc$ebnf$2"], "postprocess":  d => ({
        	type: "calcFunc",
        	funcName: d[0].value,
        	args: FORCE_ARRAY(d[3][0]),
        	swizzleRight: SAFE_VAL(d[6])
        })},
    {"name": "OperatorLR$subexpression$1", "symbols": [(lexer.has("OperatorLR") ? {type: "OperatorLR"} : OperatorLR)]},
    {"name": "OperatorLR$subexpression$1", "symbols": [(lexer.has("TypeStart") ? {type: "TypeStart"} : TypeStart)]},
    {"name": "OperatorLR$subexpression$1", "symbols": [(lexer.has("TypeEnd") ? {type: "TypeEnd"} : TypeEnd)]},
    {"name": "OperatorLR", "symbols": ["OperatorLR$subexpression$1"], "postprocess": d => d[0][0]},
    {"name": "FuncArgs", "symbols": ["FuncArg"], "postprocess": MAP_FIRST},
    {"name": "FuncArgs$subexpression$1", "symbols": ["FuncArgs", "_", (lexer.has("Seperator") ? {type: "Seperator"} : Seperator), "_", "FuncArg"]},
    {"name": "FuncArgs", "symbols": ["FuncArgs$subexpression$1"], "postprocess": d => MAP_FLATTEN_TREE(d[0], 0, 4)},
    {"name": "FuncArg", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName)], "postprocess": d => ({type: "var", value: d[0].value})},
    {"name": "FuncArg", "symbols": ["ValueNumeric"], "postprocess": d => ({type: "num", value: d[0][0]})},
    {"name": "VarList", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName)], "postprocess": MAP_FIRST},
    {"name": "VarList$subexpression$1", "symbols": ["VarList", "_", (lexer.has("Seperator") ? {type: "Seperator"} : Seperator), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName)]},
    {"name": "VarList", "symbols": ["VarList$subexpression$1"], "postprocess": d => MAP_FLATTEN_TREE(d[0], 0, 4)},
    {"name": "IndexDef", "symbols": [(lexer.has("IdxStart") ? {type: "IdxStart"} : IdxStart), "_", "ValueNumeric", "_", (lexer.has("IdxEnd") ? {type: "IdxEnd"} : IdxEnd)], "postprocess": d => d[2][0]},
    {"name": "RegDef", "symbols": [(lexer.has("TypeStart") ? {type: "TypeStart"} : TypeStart), (lexer.has("Registers") ? {type: "Registers"} : Registers), (lexer.has("TypeEnd") ? {type: "TypeEnd"} : TypeEnd)], "postprocess": d => d[1].value},
    {"name": "RegNumDef", "symbols": [(lexer.has("TypeStart") ? {type: "TypeStart"} : TypeStart), "ValueNumeric", (lexer.has("TypeEnd") ? {type: "TypeEnd"} : TypeEnd)], "postprocess": d => d[1][0]},
    {"name": "ValueNumeric$subexpression$1", "symbols": [(lexer.has("ValueBin") ? {type: "ValueBin"} : ValueBin)], "postprocess": d => parseInt(d[0].value.substring(2), 2)},
    {"name": "ValueNumeric$subexpression$1", "symbols": [(lexer.has("ValueDec") ? {type: "ValueDec"} : ValueDec)], "postprocess": d => parseInt(d[0].value, 10)},
    {"name": "ValueNumeric$subexpression$1", "symbols": [(lexer.has("ValueHex") ? {type: "ValueHex"} : ValueHex)], "postprocess": d => parseInt(d[0].value.substring(2), 16)},
    {"name": "ValueNumeric", "symbols": ["ValueNumeric$subexpression$1"]},
    {"name": "_$ebnf$1", "symbols": []},
    {"name": "_$ebnf$1", "symbols": ["_$ebnf$1", (lexer.has("_") ? {type: "_"} : _)], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "_", "symbols": ["_$ebnf$1"], "postprocess": d => null}
]
  , ParserStart: "main"
}
if (typeof module !== 'undefined'&& typeof module.exports !== 'undefined') {
   module.exports = grammar;
} else {
   window.grammar = grammar;
}
})();

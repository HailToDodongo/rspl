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

const REM_QUOTES = d => d && d.substring(1, d.length-1);
const FORCE_SCOPED_BLOCK = d => {
	if(d && d.type !== "scopedBlock") {
	   return {type: "scopedBlock", statements: [d]};
    }
	return d;
};

const moo = require("moo")
const lexer = moo.compile({
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
	  ".xyzwxyzw", ".xyzw", ".XYZW",
	  ".xy", ".zw", ".XY", ".ZW",
	  ".x", ".y", ".z", ".w", ".X", ".Y", ".Z", ".W",
	], value: s => s.substring(1)},

	FunctionType: ["function", "command", "macro"],
	KWIf      : "if",
	KWLoop    : "loop",
	KWElse    : "else",
	KWBreak   : "break",
	KWWhile   : "while",
	KWState   : "state",
	KWGoto    : "goto",
	KWExtern  : "extern",
	KWContinue: "continue",
	KWInclude : "include",
	KWConst   : "const",
	KWUndef   : "undef",
	KWExit    : "exit",
	KWAlign   : "alignas",

	ValueHex: /0x[0-9A-F']+/,
	ValueBin: /0b[0-1']+/,
	ValueFloat: /[-]?[0-9]+[.][0-9]+/,
	ValueDec: /[-]?[0-9][0-9']*/,

	OperatorSelfR: [
		"&&=", "||=",
		"&=", "|=", "^=",
		"<<=", ">>=",
		"+*=",
		"+=", "-=", "*=", "/=",
	],
	OperatorCompare: [
		"<=", ">=",
		"==", "!=",
	],
	OperatorLR: [
		"&&", "||",
		"<<", ">>",
		"+*",
		"+", "-", "*", "/",
		"&", "~|", "|", "^",
	],
	OperatorUnary: [
		"!", "~",
	],
	QuestionMark: '?',

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
	AnnoStart : "@",

	Assignment: "=",

	VarName: /[a-zA-Z0-9_]+(?:\:[a-z0-9]+)?/,
	Colon     : ":",

	_:  { match: /[ \t\n]+/, lineBreaks: true },

});
var grammar = {
    Lexer: lexer,
    ParserRules: [
    {"name": "main$ebnf$1", "symbols": []},
    {"name": "main$ebnf$1$subexpression$1", "symbols": ["_", "SectionIncl"]},
    {"name": "main$ebnf$1", "symbols": ["main$ebnf$1", "main$ebnf$1$subexpression$1"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "main$ebnf$2$subexpression$1", "symbols": ["_", "SectionState"]},
    {"name": "main$ebnf$2", "symbols": ["main$ebnf$2$subexpression$1"], "postprocess": id},
    {"name": "main$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "main$ebnf$3", "symbols": []},
    {"name": "main$ebnf$3$subexpression$1", "symbols": ["_", "Function"]},
    {"name": "main$ebnf$3", "symbols": ["main$ebnf$3", "main$ebnf$3$subexpression$1"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "main$ebnf$4", "symbols": []},
    {"name": "main$ebnf$4$subexpression$1", "symbols": ["_", "SectionIncl"]},
    {"name": "main$ebnf$4", "symbols": ["main$ebnf$4", "main$ebnf$4$subexpression$1"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "main", "symbols": ["main$ebnf$1", "main$ebnf$2", "main$ebnf$3", "main$ebnf$4", "_"], "postprocess":  d => ({
        	includes: MAP_TAKE(d[0], 1),
        	postIncludes: MAP_TAKE(d[3], 1),
        	state: (d[1] && d[1][1]) || [],
        	functions: MAP_TAKE(d[2], 1),
        }) },
    {"name": "SectionIncl", "symbols": [(lexer.has("KWInclude") ? {type: "KWInclude"} : KWInclude), "_", (lexer.has("String") ? {type: "String"} : String)], "postprocess": d => d[2].value},
    {"name": "SectionState$ebnf$1", "symbols": []},
    {"name": "SectionState$ebnf$1", "symbols": ["SectionState$ebnf$1", "StateVarDef"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "SectionState", "symbols": [(lexer.has("KWState") ? {type: "KWState"} : KWState), "_", (lexer.has("BlockStart") ? {type: "BlockStart"} : BlockStart), "_", "SectionState$ebnf$1", (lexer.has("BlockEnd") ? {type: "BlockEnd"} : BlockEnd)], "postprocess": d => d[4]},
    {"name": "StateVarDef$ebnf$1$subexpression$1", "symbols": [(lexer.has("KWExtern") ? {type: "KWExtern"} : KWExtern), "_"]},
    {"name": "StateVarDef$ebnf$1", "symbols": ["StateVarDef$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "StateVarDef$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "StateVarDef$ebnf$2", "symbols": ["StateAlign"], "postprocess": id},
    {"name": "StateVarDef$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "StateVarDef$ebnf$3", "symbols": []},
    {"name": "StateVarDef$ebnf$3", "symbols": ["StateVarDef$ebnf$3", "IndexDef"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "StateVarDef$ebnf$4", "symbols": ["StateValueDef"], "postprocess": id},
    {"name": "StateVarDef$ebnf$4", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "StateVarDef", "symbols": ["StateVarDef$ebnf$1", "StateVarDef$ebnf$2", (lexer.has("DataType") ? {type: "DataType"} : DataType), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), "StateVarDef$ebnf$3", "StateVarDef$ebnf$4", "_", (lexer.has("StmEnd") ? {type: "StmEnd"} : StmEnd), "_"], "postprocess":  d => ({
        	type: "varState",
        	extern: !!d[0],
        	varType: d[2].value,
        	varName: d[4].value,
        	arraySize: d[5] || 1,
        	align: d[1] || 0,
        	value: d[6],
        })},
    {"name": "StateValueDef", "symbols": ["_", (lexer.has("Assignment") ? {type: "Assignment"} : Assignment), "_", (lexer.has("BlockStart") ? {type: "BlockStart"} : BlockStart), "_", "NumList", "_", (lexer.has("BlockEnd") ? {type: "BlockEnd"} : BlockEnd)], "postprocess": d => d[5]},
    {"name": "StateAlign", "symbols": [(lexer.has("KWAlign") ? {type: "KWAlign"} : KWAlign), (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "ValueNumeric", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd), "_"], "postprocess": d => d[2][0]},
    {"name": "NumList", "symbols": ["ValueNumeric"], "postprocess": MAP_FIRST},
    {"name": "NumList$subexpression$1", "symbols": ["NumList", "_", (lexer.has("Seperator") ? {type: "Seperator"} : Seperator), "_", "ValueNumeric"]},
    {"name": "NumList", "symbols": ["NumList$subexpression$1"], "postprocess": d => MAP_FLATTEN_TREE(d[0], 0, 4)},
    {"name": "Function$ebnf$1$subexpression$1", "symbols": ["RegDef"]},
    {"name": "Function$ebnf$1$subexpression$1", "symbols": ["RegNumDef"]},
    {"name": "Function$ebnf$1", "symbols": ["Function$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "Function$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "Function$ebnf$2", "symbols": []},
    {"name": "Function$ebnf$2", "symbols": ["Function$ebnf$2", "FunctionDefArgs"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "Function$subexpression$1", "symbols": ["ScopedBlock"]},
    {"name": "Function$subexpression$1", "symbols": [(lexer.has("StmEnd") ? {type: "StmEnd"} : StmEnd)]},
    {"name": "Function", "symbols": [(lexer.has("FunctionType") ? {type: "FunctionType"} : FunctionType), "Function$ebnf$1", "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "_", "Function$ebnf$2", "_", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd), "Function$subexpression$1"], "postprocess": 
        d => ({
        	type: d[0].value,
        	resultType: d[1] && d[1][0],
        	name: d[3].value,
        	args: FORCE_ARRAY(d[6][0]),
        	body: d[9][0].type === "StmEnd" ? null : d[9][0],
        })
        },
    {"name": "ScopedBlock", "symbols": ["_", (lexer.has("BlockStart") ? {type: "BlockStart"} : BlockStart), "Statements", "_", (lexer.has("BlockEnd") ? {type: "BlockEnd"} : BlockEnd)], "postprocess": 
        d => ({type: "scopedBlock", statements: d[2], line: d[1].line})
        },
    {"name": "Statements$ebnf$1", "symbols": []},
    {"name": "Statements$ebnf$1$subexpression$1", "symbols": ["ScopedBlock"]},
    {"name": "Statements$ebnf$1$subexpression$1", "symbols": ["LabelDecl"]},
    {"name": "Statements$ebnf$1$subexpression$1", "symbols": ["IfStatement"]},
    {"name": "Statements$ebnf$1$subexpression$1", "symbols": ["LoopStatement"]},
    {"name": "Statements$ebnf$1$subexpression$1", "symbols": ["WhileStatement"]},
    {"name": "Statements$ebnf$1$subexpression$1", "symbols": ["Expression"]},
    {"name": "Statements$ebnf$1$subexpression$1", "symbols": ["Annotation"]},
    {"name": "Statements$ebnf$1", "symbols": ["Statements$ebnf$1", "Statements$ebnf$1$subexpression$1"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "Statements", "symbols": ["Statements$ebnf$1"], "postprocess": d => d[0].map(y => y[0])},
    {"name": "FunctionDefArgs", "symbols": ["FunctonDefArg"], "postprocess": MAP_FIRST},
    {"name": "FunctionDefArgs$subexpression$1", "symbols": ["FunctionDefArgs", "_", (lexer.has("Seperator") ? {type: "Seperator"} : Seperator), "_", "FunctonDefArg"]},
    {"name": "FunctionDefArgs", "symbols": ["FunctionDefArgs$subexpression$1"], "postprocess": d => MAP_FLATTEN_TREE(d[0], 0, 4)},
    {"name": "FunctonDefArg$ebnf$1", "symbols": ["RegDef"], "postprocess": id},
    {"name": "FunctonDefArg$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "FunctonDefArg", "symbols": [(lexer.has("DataType") ? {type: "DataType"} : DataType), "FunctonDefArg$ebnf$1", "_", (lexer.has("VarName") ? {type: "VarName"} : VarName)], "postprocess":  d => ({
        	type: d[0].value,
        	reg: d[1],
        	name: d[3] && d[3].value
        })},
    {"name": "Expression$subexpression$1", "symbols": ["ExprVarDeclAssign"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprVarDecl"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprVarUndef"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprVarAssign"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprFuncCall"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprGoto"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprContinue"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprBreak"]},
    {"name": "Expression$subexpression$1", "symbols": ["ExprExit"]},
    {"name": "Expression", "symbols": ["_", "Expression$subexpression$1", (lexer.has("StmEnd") ? {type: "StmEnd"} : StmEnd)], "postprocess": (d) => d[1][0]},
    {"name": "Annotation$ebnf$1$subexpression$1", "symbols": [(lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "AnnotationArg", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd)]},
    {"name": "Annotation$ebnf$1", "symbols": ["Annotation$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "Annotation$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "Annotation", "symbols": ["_", (lexer.has("AnnoStart") ? {type: "AnnoStart"} : AnnoStart), (lexer.has("VarName") ? {type: "VarName"} : VarName), "Annotation$ebnf$1"], "postprocess":  d => ({
        	type: "annotation",
        	name: d[2].value,
        	value: d[3] ? d[3][1] : null,
        	line: d[1].line
        
        })},
    {"name": "AnnotationArg", "symbols": ["ValueNumeric"], "postprocess": d => d[0][0]},
    {"name": "AnnotationArg", "symbols": [(lexer.has("String") ? {type: "String"} : String)], "postprocess": d => REM_QUOTES(d[0].value)},
    {"name": "LabelDecl", "symbols": ["_", (lexer.has("VarName") ? {type: "VarName"} : VarName), (lexer.has("Colon") ? {type: "Colon"} : Colon)], "postprocess": d => ({type: "labelDecl", name: d[1].value, line: d[1].line})},
    {"name": "IfStatement$subexpression$1", "symbols": ["ExprCompare"]},
    {"name": "IfStatement$subexpression$1", "symbols": ["ExprCompareBool"]},
    {"name": "IfStatement$subexpression$2", "symbols": ["ScopedBlock"]},
    {"name": "IfStatement$subexpression$2", "symbols": ["Expression"]},
    {"name": "IfStatement$ebnf$1$subexpression$1$subexpression$1", "symbols": ["ScopedBlock"]},
    {"name": "IfStatement$ebnf$1$subexpression$1$subexpression$1", "symbols": ["Expression"]},
    {"name": "IfStatement$ebnf$1$subexpression$1$subexpression$1", "symbols": ["IfStatement"]},
    {"name": "IfStatement$ebnf$1$subexpression$1", "symbols": ["_", (lexer.has("KWElse") ? {type: "KWElse"} : KWElse), "IfStatement$ebnf$1$subexpression$1$subexpression$1"]},
    {"name": "IfStatement$ebnf$1", "symbols": ["IfStatement$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "IfStatement$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "IfStatement", "symbols": ["_", (lexer.has("KWIf") ? {type: "KWIf"} : KWIf), "_", (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "IfStatement$subexpression$1", "_", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd), "IfStatement$subexpression$2", "IfStatement$ebnf$1"], "postprocess":  d => ({
        	type: "if",
        	compare: d[4][0],
        	blockIf: FORCE_SCOPED_BLOCK(d[7][0]),
        	blockElse: FORCE_SCOPED_BLOCK(d[8] && d[8][2][0]),
        	line: d[1].line
        })},
    {"name": "WhileStatement", "symbols": ["_", (lexer.has("KWWhile") ? {type: "KWWhile"} : KWWhile), "_", (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "ExprCompare", "_", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd), "ScopedBlock"], "postprocess":  d => ({
        	type: "while",
        	compare: d[4],
        	block: d[7],
        	line: d[1].line
        })},
    {"name": "LoopStatement$ebnf$1$subexpression$1", "symbols": ["_", (lexer.has("KWWhile") ? {type: "KWWhile"} : KWWhile), "_", (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "ExprCompare", "_", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd)]},
    {"name": "LoopStatement$ebnf$1", "symbols": ["LoopStatement$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "LoopStatement$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "LoopStatement", "symbols": ["_", (lexer.has("KWLoop") ? {type: "KWLoop"} : KWLoop), "ScopedBlock", "LoopStatement$ebnf$1"], "postprocess":  d => ({
        	type: "loop",
        	compare: d[3] ? d[3][4] : undefined,
        	block: d[2],
        	line: d[1].line
        })},
    {"name": "ExprVarDeclAssign$ebnf$1$subexpression$1", "symbols": [(lexer.has("KWConst") ? {type: "KWConst"} : KWConst), "__"]},
    {"name": "ExprVarDeclAssign$ebnf$1", "symbols": ["ExprVarDeclAssign$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "ExprVarDeclAssign$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprVarDeclAssign$ebnf$2", "symbols": ["RegDef"], "postprocess": id},
    {"name": "ExprVarDeclAssign$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprVarDeclAssign", "symbols": ["ExprVarDeclAssign$ebnf$1", (lexer.has("DataType") ? {type: "DataType"} : DataType), "ExprVarDeclAssign$ebnf$2", "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), "_", "ExprPartAssign"], "postprocess":  d => ({
        	type: "varDeclAssign",
        	varType: d[1].value,
        	reg: d[2], varName: d[4].value,
        	calc: d[6],
        	isConst: !!d[0],
        	line: d[1].line
        })},
    {"name": "ExprPartAssign", "symbols": [(lexer.has("Assignment") ? {type: "Assignment"} : Assignment), "_", "ExprCalcAll"], "postprocess": d => d[2][0]},
    {"name": "ExprVarDecl$ebnf$1$subexpression$1", "symbols": [(lexer.has("KWConst") ? {type: "KWConst"} : KWConst), "__"]},
    {"name": "ExprVarDecl$ebnf$1", "symbols": ["ExprVarDecl$ebnf$1$subexpression$1"], "postprocess": id},
    {"name": "ExprVarDecl$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprVarDecl$ebnf$2", "symbols": ["RegDef"], "postprocess": id},
    {"name": "ExprVarDecl$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprVarDecl", "symbols": ["ExprVarDecl$ebnf$1", (lexer.has("DataType") ? {type: "DataType"} : DataType), "ExprVarDecl$ebnf$2", "_", "VarList"], "postprocess":  d => ({
        	type: "varDeclMulti",
        	varType: d[1].value,
        	reg: d[2],
        	varNames: FORCE_ARRAY(d[4]).map(x => x.value),
        	isConst: !!d[0],
        	line: d[1].line
        })},
    {"name": "ExprVarUndef", "symbols": [(lexer.has("KWUndef") ? {type: "KWUndef"} : KWUndef), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName)], "postprocess":  d => ({
        	type: "varUndef",
        	varName: d[2].value,
        	line: d[0].line
        })},
    {"name": "ExprFuncCall$ebnf$1", "symbols": []},
    {"name": "ExprFuncCall$ebnf$1", "symbols": ["ExprFuncCall$ebnf$1", "FuncArgs"], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "ExprFuncCall", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), (lexer.has("ArgsStart") ? {type: "ArgsStart"} : ArgsStart), "_", "ExprFuncCall$ebnf$1", (lexer.has("ArgsEnd") ? {type: "ArgsEnd"} : ArgsEnd)], "postprocess":  d => ({
        	type: "funcCall",
        	func: d[0].value,
        	args: FORCE_ARRAY(d[3][0]),
        	line: d[0].line
        })},
    {"name": "ExprGoto", "symbols": [(lexer.has("KWGoto") ? {type: "KWGoto"} : KWGoto), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName)], "postprocess":  d => ({
        	type: "goto",
        	label: d[2].value,
        	line: d[2].line
        })},
    {"name": "ExprContinue", "symbols": [(lexer.has("KWContinue") ? {type: "KWContinue"} : KWContinue)], "postprocess": d => ({type: "continue", line: d[0].line})},
    {"name": "ExprBreak", "symbols": [(lexer.has("KWBreak") ? {type: "KWBreak"} : KWBreak)], "postprocess": d => ({type: "break",    line: d[0].line})},
    {"name": "ExprExit", "symbols": [(lexer.has("KWExit") ? {type: "KWExit"} : KWExit)], "postprocess": d => ({type: "exit",     line: d[0].line})},
    {"name": "ExprCompare$subexpression$1", "symbols": [(lexer.has("OperatorCompare") ? {type: "OperatorCompare"} : OperatorCompare)]},
    {"name": "ExprCompare$subexpression$1", "symbols": [(lexer.has("TypeStart") ? {type: "TypeStart"} : TypeStart)]},
    {"name": "ExprCompare$subexpression$1", "symbols": [(lexer.has("TypeEnd") ? {type: "TypeEnd"} : TypeEnd)]},
    {"name": "ExprCompare", "symbols": ["_", "FuncArg", "_", "ExprCompare$subexpression$1", "_", "FuncArg"], "postprocess":  d => ({
        	type: "compare",
        	left: d[1],
        	op: d[3][0].value,
        	right: d[5],
        	line: d[1].line
        })},
    {"name": "ExprCompareBool$ebnf$1", "symbols": [(lexer.has("OperatorUnary") ? {type: "OperatorUnary"} : OperatorUnary)], "postprocess": id},
    {"name": "ExprCompareBool$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCompareBool", "symbols": ["_", "ExprCompareBool$ebnf$1", "FuncArg"], "postprocess":  d => ({
        	type: "compare",
        	left: d[2],
        	op: (d[1] && d[1].value === "!") ? "==" : "!=",
        	right: {type: "num", value: 0},
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
    {"name": "ExprCalcAll", "symbols": ["ExprCalcCompare"]},
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
    {"name": "ExprCalcVarVar", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), "ExprCalcVarVar$ebnf$1", "_", (lexer.has("OperatorLR") ? {type: "OperatorLR"} : OperatorLR), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), "ExprCalcVarVar$ebnf$2"], "postprocess":  d => ({
        	type: "calcVarVar",
        	left: d[0].value, swizzleLeft: SAFE_VAL(d[1]),
        	op: d[3].value,
        	right: d[5].value, swizzleRight: SAFE_VAL(d[6])
        })},
    {"name": "ExprCalcVarNum$ebnf$1", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprCalcVarNum$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcVarNum", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), "ExprCalcVarNum$ebnf$1", "_", (lexer.has("OperatorLR") ? {type: "OperatorLR"} : OperatorLR), "_", "ValueNumeric"], "postprocess":  d => ({
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
    {"name": "ExprPartTernary$subexpression$1", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName)]},
    {"name": "ExprPartTernary$subexpression$1", "symbols": ["ValueNumeric"]},
    {"name": "ExprPartTernary$ebnf$1", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprPartTernary$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprPartTernary", "symbols": ["_", (lexer.has("QuestionMark") ? {type: "QuestionMark"} : QuestionMark), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName), "_", (lexer.has("Colon") ? {type: "Colon"} : Colon), "_", "ExprPartTernary$subexpression$1", "ExprPartTernary$ebnf$1", "_"], "postprocess":  d => ({
        	left: d[3].value,
        	right: d[7][0].value || d[7][0][0],
        	swizzleRight: SAFE_VAL(d[8]),
        })},
    {"name": "ExprCalcCompare$subexpression$1", "symbols": [(lexer.has("OperatorCompare") ? {type: "OperatorCompare"} : OperatorCompare)]},
    {"name": "ExprCalcCompare$subexpression$1", "symbols": [(lexer.has("TypeStart") ? {type: "TypeStart"} : TypeStart)]},
    {"name": "ExprCalcCompare$subexpression$1", "symbols": [(lexer.has("TypeEnd") ? {type: "TypeEnd"} : TypeEnd)]},
    {"name": "ExprCalcCompare$subexpression$2", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName)]},
    {"name": "ExprCalcCompare$subexpression$2", "symbols": ["ValueNumeric"]},
    {"name": "ExprCalcCompare$ebnf$1", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "ExprCalcCompare$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcCompare$ebnf$2", "symbols": ["ExprPartTernary"], "postprocess": id},
    {"name": "ExprCalcCompare$ebnf$2", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "ExprCalcCompare", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), "_", "ExprCalcCompare$subexpression$1", "_", "ExprCalcCompare$subexpression$2", "ExprCalcCompare$ebnf$1", "ExprCalcCompare$ebnf$2"], "postprocess":  d => ({
        	type: "calcCompare",
        	left: d[0].value,
        	right: d[4][0].value || d[4][0][0],
        	op: d[2][0].value,
        	swizzleRight: SAFE_VAL(d[5]),
        	ternary: d[6]
        })},
    {"name": "FuncArgs", "symbols": ["FuncArg"], "postprocess": MAP_FIRST},
    {"name": "FuncArgs$subexpression$1", "symbols": ["FuncArgs", "_", (lexer.has("Seperator") ? {type: "Seperator"} : Seperator), "_", "FuncArg"]},
    {"name": "FuncArgs", "symbols": ["FuncArgs$subexpression$1"], "postprocess": d => MAP_FLATTEN_TREE(d[0], 0, 4)},
    {"name": "FuncArg$ebnf$1", "symbols": [(lexer.has("Swizzle") ? {type: "Swizzle"} : Swizzle)], "postprocess": id},
    {"name": "FuncArg$ebnf$1", "symbols": [], "postprocess": function(d) {return null;}},
    {"name": "FuncArg", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName), "FuncArg$ebnf$1"], "postprocess": d => ({type: "var", value: d[0].value, swizzle: SAFE_VAL(d[1])})},
    {"name": "FuncArg", "symbols": ["ValueNumeric"], "postprocess": d => ({type: "num", value: d[0][0]})},
    {"name": "FuncArg", "symbols": [(lexer.has("String") ? {type: "String"} : String)], "postprocess": d => ({type: "string", value: REM_QUOTES(d[0].value)})},
    {"name": "VarList", "symbols": [(lexer.has("VarName") ? {type: "VarName"} : VarName)], "postprocess": MAP_FIRST},
    {"name": "VarList$subexpression$1", "symbols": ["VarList", "_", (lexer.has("Seperator") ? {type: "Seperator"} : Seperator), "_", (lexer.has("VarName") ? {type: "VarName"} : VarName)]},
    {"name": "VarList", "symbols": ["VarList$subexpression$1"], "postprocess": d => MAP_FLATTEN_TREE(d[0], 0, 4)},
    {"name": "IndexDef", "symbols": [(lexer.has("IdxStart") ? {type: "IdxStart"} : IdxStart), "_", "ValueNumeric", "_", (lexer.has("IdxEnd") ? {type: "IdxEnd"} : IdxEnd)], "postprocess": d => d[2][0]},
    {"name": "RegDef", "symbols": [(lexer.has("TypeStart") ? {type: "TypeStart"} : TypeStart), (lexer.has("Registers") ? {type: "Registers"} : Registers), (lexer.has("TypeEnd") ? {type: "TypeEnd"} : TypeEnd)], "postprocess": d => d[1].value},
    {"name": "RegNumDef", "symbols": [(lexer.has("TypeStart") ? {type: "TypeStart"} : TypeStart), "ValueNumeric", (lexer.has("TypeEnd") ? {type: "TypeEnd"} : TypeEnd)], "postprocess": d => d[1][0]},
    {"name": "ValueNumeric$subexpression$1", "symbols": [(lexer.has("ValueBin") ? {type: "ValueBin"} : ValueBin)], "postprocess": d =>   parseInt(d[0].value.substring(2).replaceAll("'", ""), 2)},
    {"name": "ValueNumeric$subexpression$1", "symbols": [(lexer.has("ValueFloat") ? {type: "ValueFloat"} : ValueFloat)], "postprocess": d => parseFloat(d[0].value.replaceAll("'", ""))},
    {"name": "ValueNumeric$subexpression$1", "symbols": [(lexer.has("ValueDec") ? {type: "ValueDec"} : ValueDec)], "postprocess": d =>   parseInt(d[0].value.replaceAll("'", ""), 10)},
    {"name": "ValueNumeric$subexpression$1", "symbols": [(lexer.has("ValueHex") ? {type: "ValueHex"} : ValueHex)], "postprocess": d =>   parseInt(d[0].value.substring(2).replaceAll("'", ""), 16)},
    {"name": "ValueNumeric", "symbols": ["ValueNumeric$subexpression$1"]},
    {"name": "_$ebnf$1", "symbols": []},
    {"name": "_$ebnf$1", "symbols": ["_$ebnf$1", (lexer.has("_") ? {type: "_"} : _)], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "_", "symbols": ["_$ebnf$1"], "postprocess": d => null},
    {"name": "__$ebnf$1", "symbols": [(lexer.has("_") ? {type: "_"} : _)]},
    {"name": "__$ebnf$1", "symbols": ["__$ebnf$1", (lexer.has("_") ? {type: "_"} : _)], "postprocess": function arrpush(d) {return d[0].concat([d[1]]);}},
    {"name": "__", "symbols": ["__$ebnf$1"], "postprocess": d => null}
]
  , ParserStart: "main"
}
if (typeof module !== 'undefined'&& typeof module.exports !== 'undefined') {
   module.exports = grammar;
} else {
   window.grammar = grammar;
}
})();

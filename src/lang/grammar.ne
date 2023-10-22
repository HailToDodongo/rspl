@{%
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
%}

# Pass your lexer with @lexer:
@lexer lexer

main -> (_ SectionIncl):* _ SectionState:? (_ Function):* _ {% d => ({
	includes: MAP_TAKE(d[0], 1),
	state: d[2],
	functions: MAP_TAKE(d[3], 1),
}) %}

######### Include-Section #########
SectionIncl -> %KWInclude _ %String {% d => d[2].value %}

######### State-Section #########
SectionState -> %KWState _ %BlockStart _ StateVarDef:* %BlockEnd {% d => d[4] %}
StateVarDef -> %DataType _ %VarName IndexDef:? %StmEnd _ {%
	(d) => ({type: "varState", varType: d[0].value, varName: d[2].value, arraySize: d[3] || 1})
%}

######### Function-Section #########

# Function that translates into a function/global-label in ASM...
Function -> %FunctionType (RegDef | RegNumDef):? _ %VarName %ArgsStart _ FunctionDefArgs:* _ %ArgsEnd _ %BlockStart FuncBody _ %BlockEnd {%
	d => ({
		type: d[0].value,
		resultType: d[1] && d[1][0],
		name: d[3].value,
		args: FORCE_ARRAY(d[6][0]),
		body: d[11]
	})
%}

# A functions body contains zero or more "statements"
#  -> Statements can be comments, label-declarations and expressions.
#     -> Expression can be anything that "calculates" somthing.
#        Either as a standalone function call, or by assiging something to a variable
#        The thing that is assigned can be a constant, unary or LR-expression

FuncBody -> (LineComment | LabelDecl | Expression):* {% function(d) {return {type: "funcBody", statements: d[0].map(y => y[0])}} %}
FunctionDefArgs -> FunctonDefArg {% MAP_FIRST %}
			 | (FunctionDefArgs _ %Seperator _ FunctonDefArg) {% d => MAP_FLATTEN_TREE(d[0], 0, 4) %}

FunctonDefArg -> %DataType _ %VarName {% d => ({type: d[0].value, name: d[2] && d[2].value}) %}

Expression ->  _ (ExprVarDeclAssign | ExprVarDecl | ExprVarAssign | ExprFuncCall | ExprGoto) %StmEnd {% (d) => d[1][0] %}

LabelDecl -> _ %VarName %Colon {% d => ({type: "labelDecl", name: d[1].value, line: d[1].line}) %}

######## Expressions ########
LineComment -> _ %LineComment [\n] {% (d) => ({type: "comment", comment: d[1].value, line: d[1].line}) %}

ExprVarDeclAssign -> (%DataType RegDef _ %VarName _ ExprPartAssign) {% d => ({
	type: "varDeclAssign", varType: d[0][0].value,
	reg: d[0][1], varName: d[0][3].value,
	calc: d[0][5],
	line: d[0][0].line
})%}

ExprPartAssign -> %Assignment _ ExprCalcAll {% d => d[2][0] %}

ExprVarDecl -> (%DataType RegDef _ VarList) {% d => ({
	type: "varDeclMulti",
	varType: d[0][0].value,
	reg: d[0][1],
	varNames: FORCE_ARRAY(d[0][3]).map(x => x.value),
	line: d[0][0].line
})%}

ExprFuncCall -> %VarName %ArgsStart _ (%VarName | %String) _ %ArgsEnd  {% d => ({
	type: "funcCall",
	func: d[0].value, args: d[3][0],
	line: d[0].line
})%}

ExprGoto -> %KWGoto _ %VarName {% d => ({
	type: "goto",
	label: d[2].value,
	line: d[2].line
})%}

# Assignment to a variable which calcualtes something (left-hande operator right-hand)
ExprVarAssign -> ( %VarName %Swizzle:? _ (%Assignment | %OperatorSelfR) _ ExprCalcAll) {% ([d]) => ({
	type: "varAssignCalc",
	varName: d[0].value,
	swizzle: SAFE_VAL(d[1]),
	assignType: d[3][0].value,
	calc: d[5][0],
	line: d[0].line
})%}

#### Calculations ####
ExprCalcAll -> ExprCalcVarVar | ExprCalcVarNum | ExprCalcNum | ExprCalcVar | ExprCalcFunc

ExprCalcNum -> ValueNumeric {% d => ({type: "calcNum", right: d[0][0]}) %}

ExprCalcVar -> %OperatorUnary:? %VarName %Swizzle:? {% d => ({
	type: "calcVar",
	op: SAFE_VAL(d[0]),
	right: d[1].value, swizzleRight: SAFE_VAL(d[2])
})%}

ExprCalcVarVar -> %VarName %Swizzle:? _ OperatorLR _ %VarName %Swizzle:? {% d => ({
	type: "calcVarVar",
	left: d[0].value, swizzleLeft: SAFE_VAL(d[1]),
	op: d[3].value,
	right: d[5].value, swizzleRight: SAFE_VAL(d[6])
})%}

ExprCalcVarNum -> %VarName %Swizzle:? _ OperatorLR _ ValueNumeric {% d => ({
	type: "calcVarNum",
	left: d[0].value, swizzleLeft: SAFE_VAL(d[1]),
	op: d[3].value,
	right: d[5][0]
})%}

ExprCalcFunc -> %VarName %ArgsStart _ FuncArgs:* _ %ArgsEnd %Swizzle:? {% d => ({
	type: "calcFunc",
	funcName: d[0].value,
	args: FORCE_ARRAY(d[3][0]),
	swizzleRight: SAFE_VAL(d[6])
})%}

OperatorLR -> (%OperatorLR | %TypeStart | %TypeEnd) {% d => d[0][0] %} # "<" and ">" are overloaded (comparision & type-spec)

######## Arguments ########
FuncArgs -> FuncArg {% MAP_FIRST %}
			 | (FuncArgs _ %Seperator _ FuncArg) {% d => MAP_FLATTEN_TREE(d[0], 0, 4) %}

FuncArg -> %VarName {% d => ({type: "var", value: d[0].value}) %}
	 | ValueNumeric {% d => ({type: "num", value: d[0][0]}) %}

VarList -> %VarName {% MAP_FIRST %}
			 | (VarList _ %Seperator _ %VarName) {% d => MAP_FLATTEN_TREE(d[0], 0, 4) %}

######## Arrays ########
IndexDef -> %IdxStart _ ValueNumeric _ %IdxEnd {% d => d[2][0] %}

######## Registers ########
RegDef    -> %TypeStart %Registers   %TypeEnd {% d => d[1].value %}
RegNumDef -> %TypeStart ValueNumeric %TypeEnd {% d => d[1][0] %}

######## Values ########
ValueNumeric -> (
  %ValueBin {% d => parseInt(d[0].value.substring(2), 2) %} |
  %ValueDec {% d => parseInt(d[0].value, 10) %} |
  %ValueHex {% d => parseInt(d[0].value.substring(2), 16) %}
)

_ -> %_:* {% d => null %}
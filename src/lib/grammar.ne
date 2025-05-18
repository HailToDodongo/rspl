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

const REM_QUOTES = d => d && d.substring(1, d.length-1);
const FORCE_SCOPED_BLOCK = d => {
	if(d && d.type !== "scopedBlock") {
	   return {type: "scopedBlock", statements: [d]};
    }
	return d;
};

const SWIZZLE_ALIAS_MAP = {
	"0": "x", "1": "y", "2": "z", "3": "w",
	"4": "X", "5": "Y", "6": "Z", "7": "W",
};

const NORM_SWIZZLE = s => s.split("")
	.map(l => SWIZZLE_ALIAS_MAP[l] || l)
	.join("");

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

	  ".00224466", ".11335577",
	  ".00004444", ".11115555", ".22226666", ".33337777",
	  ".00000000", ".11111111", ".22222222", ".33333333",
	  ".44444444", ".55555555", ".66666666", ".77777777",
	  ".01230123", ".0123", ".4567",
	  ".01", ".23", ".45", ".67",
	  ".0", ".1", ".2", ".3", ".4", ".5", ".6", ".7",
	], value: s => NORM_SWIZZLE(s.substring(1))},

	FunctionType: ["function", "command", "macro"],
	KWIf      : "if",
	KWLoop    : "loop",
	KWElse    : "else",
	KWBreak   : "break",
	KWWhile   : "while",
	KWGoto    : "goto",
	KWExtern  : "extern",
	KWContinue: "continue",
	KWInclude : "include",
	KWConst   : "const",
	KWUndef   : "undef",
	KWExit    : "exit",
	KWAlign   : "alignas",

	ValueHex: /0x[0-9A-Fa-f']+/,
	ValueBin: /0b[0-1']+/,
	ValueFloat: /[-]?[0-9]+[.][0-9]+/,
	ValueDec: /[-]?[0-9][0-9']*/,

	OperatorSelfR: [
		"&&=", "||=",
		"&=", "|=", "^=",
		">>>=",
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
		">>>", ">>", "<<",
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
%}

# Pass your lexer with @lexer:
@lexer lexer

main -> (_ SectionIncl):* (_ SectionState):* (Function):* (_ SectionIncl):* _ {% d => ({
	includes: MAP_TAKE(d[0], 1),
	states: MAP_TAKE(d[1], 1),
	functions: MAP_TAKE(d[2], 0),
	postIncludes: MAP_TAKE(d[3], 1),
}) %}

######### Include-Section #########
SectionIncl -> %KWInclude _ %String {% d => d[2].value %}

######### State-Section #########
SectionState -> %VarName _ %BlockStart _ StateVarDef:* %BlockEnd {% d => ({
	name: d[0].value,
	vars: d[4]
}) %}

StateVarDef -> (%KWExtern _):? StateAlign:? %DataType _ %VarName IndexDef:* StateValueDef:? _ %StmEnd _ {% d => ({
	type: "varState",
	extern: !!d[0],
	varType: d[2].value,
	varName: d[4].value,
	arraySize: d[5] || 1,
	align: d[1] || 0,
	value: d[6],
})%}

StateValueDef -> _ %Assignment _ %BlockStart _ NumList _ %BlockEnd {% d => d[5] %}

StateAlign -> %KWAlign %ArgsStart ValueNumeric %ArgsEnd _ {% d => d[2][0] %}

NumList -> ValueNumeric {% MAP_FIRST %}
		  | (NumList _ %Seperator _ ValueNumeric) {% d => MAP_FLATTEN_TREE(d[0], 0, 4) %}


######### Function-Section #########

# Function that translates into a function/global-label in ASM...
Function -> Annotation:* _ %FunctionType (RegDef | RegNumDef):? _ %VarName %ArgsStart _ FunctionDefArgs:* _ %ArgsEnd (ScopedBlock | %StmEnd)  {%
	d => ({
		annotations: d[0],
		type: d[2].value,
		resultType: d[3] && d[3][0],
		name: d[5].value,
		args: FORCE_ARRAY(d[8][0]),
		body: d[11][0].type === "StmEnd" ? null : d[11][0],
	})
%}

ScopedBlock -> _ %BlockStart Statements _ %BlockEnd {%
	d => ({type: "scopedBlock", statements: d[2], line: d[1].line})
%}

# A functions body (or any scoped-block) contains zero or more "statements"
#  -> Statements can be comments, label-declarations and expressions.
#     -> Expression can be anything that "calculates" somthing.
#        Either as a standalone function call, or by assiging something to a variable
#        The thing that is assigned can be a constant, unary or LR-expression

Statements -> (ScopedBlock | LabelDecl | IfStatement | LoopStatement | WhileStatement | Expression  | Annotation):* {% d => d[0].map(y => y[0]) %}
FunctionDefArgs -> FunctonDefArg {% MAP_FIRST %}
			 | (FunctionDefArgs _ %Seperator _ FunctonDefArg) {% d => MAP_FLATTEN_TREE(d[0], 0, 4) %}

FunctonDefArg -> %DataType RegDef:? _ %VarName {% d => ({
	type: d[0].value,
	reg: d[1],
	name: d[3] && d[3].value
})%}

Expression ->  _ (ExprVarDeclAssign | ExprVarDecl | ExprVarUndef | ExprVarAssign | ExprFuncCall | ExprGoto | ExprContinue | ExprBreak | ExprExit) %StmEnd {% (d) => d[1][0] %}

Annotation -> _ %AnnoStart %VarName (%ArgsStart AnnotationArg %ArgsEnd):? {% d => ({
	type: "annotation",
	name: d[2].value,
	value: d[3] ? d[3][1] : null,
	line: d[1].line

})%}

AnnotationArg -> ValueNumeric {% d => d[0][0] %}
	 | %String {% d => REM_QUOTES(d[0].value) %}

LabelDecl -> _ %VarName %Colon {% d => ({type: "labelDecl", name: d[1].value, line: d[1].line}) %}

IfStatement -> _ %KWIf _ %ArgsStart (ExprCompare | ExprCompareBool) _ %ArgsEnd (ScopedBlock | Expression) (_ %KWElse (ScopedBlock | Expression | IfStatement)):? {% d => ({
	type: "if",
	compare: d[4][0],
	blockIf: FORCE_SCOPED_BLOCK(d[7][0]),
	blockElse: FORCE_SCOPED_BLOCK(d[8] && d[8][2][0]),
	line: d[1].line
})%}

WhileStatement -> _ %KWWhile _ %ArgsStart ExprCompare _ %ArgsEnd ScopedBlock {% d => ({
	type: "while",
	compare: d[4],
	block: d[7],
	line: d[1].line
})%}


LoopStatement -> _ %KWLoop ScopedBlock (_ %KWWhile _ %ArgsStart ExprCompare _ %ArgsEnd):? {% d => ({
	type: "loop",
	compare: d[3] ? d[3][4] : undefined,
	block: d[2],
	line: d[1].line
})%}

######## Expressions ########
ExprVarDeclAssign -> (%KWConst __):? %DataType RegDef:? _ %VarName _ ExprPartAssign {% d => ({
	type: "varDeclAssign",
	varType: d[1].value,
	reg: d[2], varName: d[4].value,
	calc: d[6],
	isConst: !!d[0],
	line: d[1].line
})%}

ExprPartAssign -> %Assignment _ ExprCalcAll {% d => d[2][0] %}

ExprVarDecl -> (%KWConst __):? %DataType RegDef:? _ VarList {% d => ({
	type: "varDeclMulti",
	varType: d[1].value,
	reg: d[2],
	varNames: FORCE_ARRAY(d[4]).map(x => x.value),
	isConst: !!d[0],
	line: d[1].line
})%}

ExprVarUndef -> %KWUndef _ %VarName {% d => ({
	type: "varUndef",
	varName: d[2].value,
	line: d[0].line
})%}

ExprFuncCall -> %VarName %ArgsStart _ FuncArgs:* %ArgsEnd  {% d => ({
	type: "funcCall",
	func: d[0].value,
	args: FORCE_ARRAY(d[3][0]),
	line: d[0].line
})%}

ExprGoto -> %KWGoto _ %VarName {% d => ({
	type: "goto",
	label: d[2].value,
	line: d[2].line
})%}

ExprContinue -> %KWContinue {% d => ({type: "continue", line: d[0].line})%}
ExprBreak    -> %KWBreak    {% d => ({type: "break",    line: d[0].line})%}
ExprExit     -> %KWExit     {% d => ({type: "exit",     line: d[0].line})%}

ExprCompare -> _ FuncArg _ (%OperatorCompare | %TypeStart | %TypeEnd) _ FuncArg {% d => ({
	type: "compare",
	left: d[1],
	op: d[3][0].value,
	right: d[5],
	line: d[1].line
})%}

ExprCompareBool -> _ %OperatorUnary:? FuncArg {% d => ({
	type: "compare",
	left: d[2],
	op: (d[1] && d[1].value === "!") ? "==" : "!=",
	right: {type: "num", value: 0},
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
ExprCalcAll -> ExprCalcVarVar | ExprCalcVarNum | ExprCalcNum | ExprCalcVar | ExprCalcFunc | ExprCalcCompare

ExprCalcNum -> ValueNumeric {% d => ({type: "calcNum", right: d[0][0]}) %}

ExprCalcVar -> %OperatorUnary:? %VarName %Swizzle:? {% d => ({
	type: "calcVar",
	op: SAFE_VAL(d[0]),
	right: d[1].value, swizzleRight: SAFE_VAL(d[2])
})%}

ExprCalcVarVar -> %VarName %Swizzle:? _ %OperatorLR _ %VarName %Swizzle:? {% d => ({
	type: "calcVarVar",
	left: d[0].value, swizzleLeft: SAFE_VAL(d[1]),
	op: d[3].value,
	right: d[5].value, swizzleRight: SAFE_VAL(d[6])
})%}

ExprCalcVarNum -> %VarName %Swizzle:? _ %OperatorLR _ ValueNumeric {% d => ({
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

ExprPartTernary -> _ %QuestionMark _ %VarName _ %Colon _ (%VarName | ValueNumeric) %Swizzle:? _ {% d => ({
	left: d[3].value,
	right: d[7][0].value || d[7][0][0],
	swizzleRight: SAFE_VAL(d[8]),
})%}

ExprCalcCompare -> %VarName _ (%OperatorCompare | %TypeStart | %TypeEnd) _ (%VarName | ValueNumeric) %Swizzle:? ExprPartTernary:? {% d => ({
	type: "calcCompare",
	left: d[0].value,
	right: d[4][0].value || d[4][0][0],
	op: d[2][0].value,
	swizzleRight: SAFE_VAL(d[5]),
	ternary: d[6]
})%}

######## Arguments ########
FuncArgs -> FuncArg {% MAP_FIRST %}
			 | (FuncArgs _ %Seperator _ FuncArg) {% d => MAP_FLATTEN_TREE(d[0], 0, 4) %}

FuncArg -> %VarName %Swizzle:? {% d => ({type: "var", value: d[0].value, swizzle: SAFE_VAL(d[1])}) %}
	 | ValueNumeric {% d => ({type: "num", value: d[0][0]}) %}
	 | %String {% d => ({type: "string", value: REM_QUOTES(d[0].value)}) %}


VarList -> %VarName {% MAP_FIRST %}
			 | (VarList _ %Seperator _ %VarName) {% d => MAP_FLATTEN_TREE(d[0], 0, 4) %}

######## Arrays ########
IndexDef -> %IdxStart _ ValueNumeric _ %IdxEnd {% d => d[2][0] %}

######## Registers ########
RegDef    -> %TypeStart %Registers   %TypeEnd {% d => d[1].value %}
RegNumDef -> %TypeStart ValueNumeric %TypeEnd {% d => d[1][0] %}

######## Values ########
ValueNumeric -> (
  %ValueBin    {% d =>   parseInt(d[0].value.substring(2).replaceAll("'", ""), 2)  %} |
  %ValueFloat  {% d => parseFloat(d[0].value.replaceAll("'", "")) 				   %} |
  %ValueDec    {% d =>   parseInt(d[0].value.replaceAll("'", ""), 10)			   %} |
  %ValueHex    {% d =>   parseInt(d[0].value.substring(2).replaceAll("'", ""), 16) %}
)

_ -> %_:* {% d => null %}
__ -> %_:+ {% d => null %}

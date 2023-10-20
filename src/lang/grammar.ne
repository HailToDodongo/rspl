@{% 
	const MAP_NULL = () => null;
	const MAP_FIRST = d => d[0]; 
	const MAP_ENUM = d => d[0][0]; 
	const MAP_TAKE = (d, i) => d && d.map(x => x[i]);
	const MAP_FLATTEN_TREE = (d, idxLeft, idxRight) => [
		...(Array.isArray(d[idxLeft]) ? d[idxLeft] : [d[idxLeft]]),
		...(Array.isArray(d[idxRight]) ? d[idxRight] : [d[idxRight]])
	];
	const FORCE_ARRAY = d => Array.isArray(d) ? d : (d ? [d] : []);
%}

# A File contains 2 main sections:
# "State"    : defines variables in RAM that need to be saved across code-switches
# "Functions": can contain one or more functions (either commands or regular functions)
File -> (_ SectionState):? (_ Function):* _ {%
	d => ({
   		state    : d[0] && d[0][1],
   		functions: MAP_TAKE(d[1], 1)
  	})
%}

######### State-Section #########
SectionState -> "State"i _ "{" _ StateVarDef:* "}" {% d => d[4] %}
StateVarDef -> DataType _ VarName _ ";" _  {%
	(d) => ({type: "varState", varType: d[0], varName: d[2]})
%}

######### Function-Section #########

# Function that translates into a function/global-label in ASM...
Function -> FunctionType _ FuncName "(" _ FunctionArgs:* _ ")" _ "{" FuncBody _ "}" {%
	d => ({
		...d[0],
		name: d[2],
		args: FORCE_ARRAY(d[5][0]),
		body: d[10]
	})
%}

FunctonArg -> DataType _ VarName {% d => ({type: d[0], name: d[2]}) %}
FunctionArgs -> FunctonArg {% MAP_FIRST %}
			 | (FunctionArgs _ "," _ FunctonArg) {% d => MAP_FLATTEN_TREE(d[0], 0, 4) %}

# ...which contains a list of statements or comments
FuncBody -> (Statement | LineComment):* {% function(d) {return {type: "funcBody", statements: d[0].map(y => y[0])}} %}
# Each statement can be a var-declaration, assignment (with calculations) or inline-asm
Statement -> _ (ExprVarDeclAssign | ExprVarAssign) ";" {% (d) => d[1][0] %}

FunctionType -> (("command"i RegNumDef) | ("function"i RegDef:?)) {%
	d => ({type: d[0][0][0], resultType: d[0][0][1]})
%}

##### All Expressions #####

LineComment -> _ "//" .:* [\n] {% (d) => ({type: "comment", comment: d[2].join("")}) %}

# Declaring a variable with an optional assignment
ExprVarDeclAssign -> (DataType RegDef _ VarName _ ExprPartAssign:?) {%
	([d]) => ({type: "varDeclAssign", varType: d[0], reg: d[1], varName: d[3], value: d[5]})
%}

ExprPartAssign -> "=" _ (ValueNumeric | ExprFuncCall) _ OpsSwizzle:? {% d => ({type: "value", value: d[2][0][0], swizzle: d[4]}) %}

ExprFuncCall -> FuncName "(" _ VarName _ ")"  {% d => ({type: "funcCall", func: d[0], args: d[3]}) %}

# Raw inline ASM
# ExprASM -> "asm(\"" [a-zA-Z0-9,_$%\)\( ]:* "\")" {% d => ({type: "asm", asm: d[1].join("")}) %}

# Assignment to a variable which calcualtes something (left-hande operator right-hand)
ExprVarAssign -> ( VarName _ "=" _ (ExprCalcVarVar | ExprCalcVarNum)) {%
	([d]) => ({type: "varAssignCalc", varName: d[0], value: d[5], calc: d[4][0]})
%}

#### Calculations ####
ExprCalcVarVar -> VarName OpsSwizzle:? _ OpsLeftRight _ VarName OpsSwizzle:? {%
	d => ({type: "calcVarVar", left: d[0], swizzleLeft: d[1], op: d[3], right: d[5], swizzleRight: d[6]})
%}

ExprCalcVarNum -> VarName OpsSwizzle:? _ OpsLeftRight _ ValueNumeric {%
	d => ({type: "calcVarNum", left: d[0], swizzleLeft: d[1], op: d[3], right: d[5]})
%}

#### Names & Keywords ####)
VarName -> [a-zA-Z] [a-zA-Z0-9_]:* {% d => d[0][0] + d[1].join("") %}
FuncName -> [a-zA-Z0-9_]:+ {% d => d[0].join("") %}

#### Data Types ####
DataType -> (
	"u8" | "s8" | "u16" | "s16" | "u32" | "s32" |
	"vec32" | "vec16"
) {% MAP_ENUM %}

#### Registers ####
RegsAll -> RegsScalar | RegsVector
RegsScalar -> (
	"$at" | "$zero" | "$v0" | "$v1" | "$a0" | "$a1" | "$a2" | "$a3" |
	"$t0" | "$t1" | "$t2" | "$t3" | "$t4" | "$t5" | "$t6" | "$t7" | "$t8" | "$t9" |
	"$s0" | "$s1" | "$s2" | "$s3" | "$s4" | "$s5" | "$s6" | "$s7" |
	"$k0" | "$k1" | "$gp" | "$sp" | "$fp" | "$ra"
) {% MAP_ENUM %}

RegsVector -> (
	"$v00" | "$v01" | "$v02" | "$v03" | "$v04" | "$v05" | "$v06" | "$v07" |
	"$v08" | "$v09" | "$v10" | "$v11" | "$v12" | "$v13" | "$v14" | "$v15" |
	"$v16" | "$v17" | "$v18" | "$v19" | "$v20" | "$v21" | "$v22" | "$v23" |
	"$v24" | "$v25" | "$v26" | "$v27" | "$v28" | "$v29" | "$v30" | "$v31"
) {% MAP_ENUM %}

RegDef -> "<" RegsAll ">" {% (d) => d[1][0] %}
RegNumDef -> "<" ValueNumeric ">" {% (d) => d[1] %}

#### Values ####
ValueNumeric -> (ValueInt | ValueHex) {% (d) => d[0][0] %}
ValueInt -> [0-9]:+         {% (d) => parseInt(d[0].join("")) %}
ValueHex -> "0x" [0-9A-F]:+ {% (d) => parseInt(d[1].join(""), 16) %}

#### Operators ####
OpsLeftRight -> (OpsNumeric | OpsLogic | OpsBitwise) {% MAP_ENUM %}
OpsNumeric -> ("+" | "-" | "*" | "+*" | "/") {% MAP_ENUM %}
OpsLogic   -> ("&&" | "||" | "!" | "==" | "!=" | "<" | ">" | "<=" | ">=") {% MAP_ENUM %}
OpsBitwise -> ("&" | "|" | "^" | "~" | "<<" | ">>") {% MAP_ENUM %}

OpsSwizzle -> (
  ".xxzzXXZZ" | ".yywwYYWW" |
  ".xxxxXXXX" | ".yyyyYYYY" | ".zzzzZZZZ" | ".wwwwWWWW" |
  ".xxxxxxxx" | ".yyyyyyyy" | ".zzzzzzzz" | ".wwwwwwww" |
  ".XXXXXXXX" | ".YYYYYYYY" | ".ZZZZZZZZ" | ".WWWWWWWW"
) {% d => d[0][0].substring(1) %}

#### Whitespace ####
_ -> [\s]:*  {% MAP_NULL %}

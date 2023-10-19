@{% 
	const MAP_NULL = () => null;
	const MAP_FIRST = d => d[0]; 
	const MAP_ENUM = d => d[0][0]; 
	const MAP_TAKE = (d, i) => d && d.map(x => x[i]);

%}

# A File contains 3 main sections:
# "Commands" : define the public API by connection an index with a function
# "State"    : defines variables in RAM that need to be saved across code-switches
# "Functions": can contain one or more functions (either commands or regular functions)
File -> (_ SectionCmd):? (_ SectionState):? (_ Function):* _ {% 
	d => ({
		commands : d[0] && d[0][1],
   		state    : d[1] && d[1][1],
   		functions: MAP_TAKE(d[2], 1)
  	}) 
%}

######### Command-Section #########
SectionCmd -> "Commands"i _ "{" _ CommandDef:* "}" {% d => d[4] %}
CommandDef -> ValueNumeric ":" _ VarName _ {% d => ({idx: d[0], name: d[3]}) %}

######### State-Section #########
SectionState -> "State"i _ "{" _ StateVarDef:* "}" {% d => d[4] %}
StateVarDef -> DataType _ VarName _ ";" _  {% 
	(d) => ({type: "varState", varType: d[0], varName: d[2]})
%}

######### Function-Section #########

# Function that translates into a function/global-label in ASM...
Function -> FuncName "()" _LB_ "{" FuncBody _LB_ "}" {% d => ({type: "function", name: d[0], body: d[4]}) %}
# ...which contains a list of statements or comments
FuncBody -> (Statement | LineComment):* {% function(d) {return {type: "funcBody", statements: d[0].map(y => y[0])}} %}
# Each statement can be a var-declaration, assignment (with calculations) or inline-asm
Statement -> _ (ExprVarDeclAssign | ExprVarAssign | ExprASM) ";" {% (d) => d[1][0] %}

##### All Expressions #####

LineComment -> _ "//" .:* [\n] {% (d) => ({type: "comment", comment: d[2].join("")}) %}

# Declaring a variable with an optional assignment
ExprVarDeclAssign -> (DataType RegDef _ VarName _ ExprPartAssign:?) {% 
	([d]) => ({type: "varDeclAssign", varType: d[0], reg: d[1], varName: d[3], value: d[5]})
%}

ExprPartAssign -> "=" _ (ValueNumeric | ExprFuncCall) _ OpsSwizzle:? {% d => ({type: "value", value: d[2][0], swizzle: d[4]}) %}

ExprFuncCall -> FuncName "(" _ VarName _ ")"  {% d => ({type: "funcCall", func: d[0], args: d[3]}) %}

# Raw inline ASM
ExprASM -> "asm(\"" [a-zA-Z0-9 ]:* "\")" {% d => ({type: "asm", asm: d[1].join("")}) %}

# Assignment to a variable which calcualtes something (left-hande operator right-hand)
ExprVarAssign -> ( VarName _ "=" _ ExprPartAssignCalc) {% 
	([d]) => ({type: "varAssignCalc", varName: d[0], value: d[5], calc: d[4]})
%}

ExprPartAssignCalc -> VarName OpsSwizzle:? _ OpsLeftRight _ VarName OpsSwizzle:? {% 
	d => ({type: "calc", left: d[0], swizzleLeft: d[1], op: d[3], right: d[5], swizzleRight: d[6]}) 
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

#### Values ####
ValueNumeric -> (ValueInt | ValueHex) {% (d) => d[0][0] %}
ValueInt -> [0-9]:+         {% (d) => parseInt(d[0].join("")) %}
ValueHex -> "0x" [0-9A-F]:+ {% (d) => parseInt(d[1].join(""), 16) %}

#### Operators ####
OpsLeftRight -> (OpsNumeric | OpsLogic | OpsBitwise) {% MAP_ENUM %}
OpsNumeric -> ("+" | "-" | "*" | "+*" | "/" | ".") {% MAP_ENUM %}
OpsLogic   -> ("&&" | "||" | "!" | "==" | "!=" | "<" | ">" | "<=" | ">=") {% MAP_ENUM %}
OpsBitwise -> ("&" | "|" | "^" | "~" | "<<" | ">>") {% MAP_ENUM %}

OpsSwizzle -> (".xxxxXXXX" | ".yyyyYYYY" | ".zzzzZZZZ" | ".wwwwWWWW") {% MAP_ENUM %}

#### Whitespace ####
_ -> [\s]:*  {% MAP_NULL %}
_LB_ -> [ \t\n]:* {% MAP_NULL %}


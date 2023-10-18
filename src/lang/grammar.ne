@{% 
	const MAP_NULL = () => null;
	const MAP_FIRST = d => d[0]; 
%}

Function -> __ FuncName "()" _LB_ "{" FuncBody _LB_ "}" {% (d) => ({type: "function", name: d[1], body: d[5]}) %}

FuncBody -> ( _ Statement _ ";"):* {% function(d) {return {type: "funcBody", statements: d[0].map(y => y[1])}} %}

Statement -> (ExprVarDeclAssign | ExprVarAssign | ExprASM) {% (d) => d[0][0] %}

##### All Expressions #####

# Declaring a variable with an optional assignment
ExprVarDeclAssign -> (DataType RegDef __ VarName __ ExprPartAssign:?) {% 
	([d]) => ({type: "varDeclAssign", varType: d[0], reg: d[1], varName: d[3], value: d[5]})
%}

ExprPartAssign -> "=" __ (ValueNumeric | ExprFuncCall) __ OpsSwizzle:? {% d => ({type: "value", value: d[2][0], swizzle: d[4]}) %}

ExprFuncCall -> FuncName "(" __ VarName __ ")"  {% d => ({type: "funcCall", func: d[0], args: d[3]}) %}

# Raw inline ASM
ExprASM -> "asm(\"" [a-zA-Z0-9 ]:* "\")" {% d => ({type: "asm", asm: d[1].join("")}) %}

ExprVarAssign -> ( VarName __ "=" __ ExprPartAssignCalc) {% 
	([d]) => ({type: "varAssignCalc", varName: d[0], value: d[5], calc: d[4]})
%}
ExprPartAssignCalc -> VarName __ OpsNumeric __ VarName OpsSwizzle:?



#### Names & Keywords ####)
VarName -> [a-zA-Z0-9_]:+ {% d => d[0].join("") %}
FuncName -> [a-zA-Z0-9_]:+ {% d => d[0].join("") %}

#### Data Types ####
DataType -> ("u32" | "s32" | "vec32" | "vec16") {% (d) => d[0][0] %}

#### Registers ####
RegsAll -> RegsScalar | RegsVector
RegsScalar -> ("$t0" | "$t1") {% (d) => d[0][0] %}
RegsVector -> ("$v0" | "$v1") {% (d) => d[0][0] %}
		  
RegDef -> "<" RegsAll ">" {% (d) => d[1][0] %}

#### Values ####
ValueNumeric -> (ValueInt | ValueHex) {% (d) => d[0][0] %}
ValueInt -> [0-9]:+         {% (d) => parseInt(d[0].join("")) %}
ValueHex -> "0x" [0-9A-F]:+ {% (d) => parseInt(d[1].join(""), 16) %}

#### Operators ####
OpsNumeric -> ("+" | "-" | "*" | "+*" | "/" | ".")
OpsSwizzle -> (".xxxxXXXX" | ".yyyyYYYY" | ".zzzzZZZZ" | ".wwwwWWWW") {% (d) => d[0][0] %}

#### Whitespace ####
_ -> [\s]:*  {% MAP_NULL %}
__ -> [ +]:* {% MAP_NULL %}
_LB_ -> [ \t\n]:* {% MAP_NULL %}

/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

export function astNormalizeFunctions(ast)
{
  const astFunctions = ast.functions;
  for(const block of astFunctions) {
    if(!["function", "command"].includes(block.type))continue;

    const statements = [];
    for(const st of block.body.statements) 
    {
      // Split up declaration and assignment
      if(st.type === "varDeclAssign") {
        statements.push({...st, type: "varDecl"});
        if(st.calc) { // ... and ignore empty assignments
          statements.push({type: "varAssignCalc", varName: st.varName, calc: st.calc});
        }
      } else {
        statements.push(st);
      }
    }

    // convert constants from seemingly being variables to immediate-values
    // this changes the calc. type, instructions need to handle both numbers and strings
    for(const st of statements) {
      if(st.type === "varAssignCalc" && (["calcVar", "calcVarVar"].includes(st.calc.type))) {
        const stateVar = ast.state.find(s => s.varName === st.calc.right);
        if(stateVar) {
          st.calc.type = st.calc.type === "calcVar" ? "calcNum" : "calcVarNum";
          st.calc.right = `%lo(${st.calc.right})`;
        }
      }

    }

    block.body.statements = statements;
  }

  return astFunctions;
}
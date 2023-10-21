/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

export function astNormalize(astFunctions)
{
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

    block.body.statements = statements;
  }

  return astFunctions;
}
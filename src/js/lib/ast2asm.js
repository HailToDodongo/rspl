/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/

const toHex16 = v => "0x" + v.toString(16).padStart(4, '0').toUpperCase();
const toHex32 = v => "0x" + v.toString(16).padStart(8, '0').toUpperCase();

function functionToAsm(func)
{
  const res = [];
  
  const varMap = {};
  const regVarMap = {};

  for(const st of func.statements) 
  {
    switch(st.type) 
    {
      case "comment":
        res.push(["##" + (st.comment || "")]);
      break;

      case "varDecl":
        // @TODO: check for conflicts
        varMap[st.varName] = {reg: st.reg, type: st.varType};
        regVarMap[st.reg] = st.varName;
        break;

      case "varAssign": {
        if(st.value.type !== "value") {
          throw new Error("Invalid value! " + JSON.stringify(st));
        }

        const refVar = varMap[st.varName];
        console.log("refVar", refVar);
        const val = st.value.value;
        res.push(["li", refVar.reg, val > 0xFFFF ? toHex32(val.toString(16)) : `%lo(${toHex16(val)})`]);
      } break;

      default:
        res.push(["#### UNKNOWN: " + JSON.stringify(st)])
    }
  }
  return res;
}

export function ast2asm(ast)
{
  const res = [];
  console.log(ast);

  for(const block of ast.functions)
  {
    if(block.type === "function") {
      console.log(block);

      res.push({
        type: "function",
        name: block.name,
        asm: functionToAsm(block.body)
      });
    }
  }

  return res;
}

import state from "./state.js";

/**
 * Applies order of operations by putting brackets around things
 *
 * @param {Array<string|{val:ASTExprNum | ASTExprVarName, swizzle:Swizzle}>} parts
 */
function applyPrecedence(parts, level = 0) {
  const precedence = [
    ["*", "/"],
    ["+", "-"],
    ["<<", ">>", ">>>"],
    ["&"],
    ["^"],
    ["|"]
  ];

  for(const ops of precedence)
  {
    let idx = -1;
    for(let i=0; i<=parts.length; ++i)
    {
      const part = parts[i] || " ";
      if(typeof(part) !== "string"){
        if(Array.isArray(part) && level === 0) {
          parts[i] = applyPrecedence(part, level + 1);
        }
        continue;
      }

      // found the first operation that uses the current precedence
      const isPrecOp = ops.includes(part);
      if(idx === -1 && isPrecOp) {
        idx = i-1;
      }
      if(idx >= 0 && !isPrecOp) {
        // create sub-array and insert into current array
        if(i !== parts.length-1)  {
          const newParts = parts.slice(idx, i);
          parts.splice(idx, i - idx, newParts);
          i = idx+1;
        }
        idx = -1;
      }
    }
  }
  return parts;
}

/**
 * Converts brackets into nested arrays.
 * E.g. [a, "+", "(" , b, "*", "c", ")"]
 * Into: [ a, "+", [ b, "*", "c"]]
 * @param {Array<string|{val:ASTExprNum | ASTExprVarName, swizzle:Swizzle}>} parts
 */
function partsToTree(parts, groupLevel = 0) {
  let idx = -1;
  for(let i = 0; i < parts.length; i++) {
    const part = parts[i];
    if(part === "(") {
      groupLevel++;
      idx = i;
    } else if(part === ")") {
      groupLevel--;
      if(groupLevel < 0) {
        state.throwError("Unmatched closing bracket!", part);
      }
      if(idx !== -1) {
        const newParts = parts.slice(idx + 1, i);
        parts.splice(idx, i - idx + 1, newParts);
        return partsToTree(parts, groupLevel);
      }
    }
  }
  return parts;
}

/**
 * Tries to evaluate parts at compile-time that only contain constants
 * E.g. [a, "+", [3, '*', 4]]
 * Into: [ a, "+", 12]
 * @param {Array<string|{val:ASTExprNum | ASTExprVarName, swizzle:Swizzle}>} parts
 */
function partsEval(parts, level = 0) {

  for(let i = 0; i < parts.length; i++)
  {
    if(Array.isArray(parts[i])) {
      parts[i] = partsEval(parts[i], level + 1);
      if(!Array.isArray(parts[i])) {
        i = -1; // bracket was completely evaluated into single value
      }
    } else {
      let valueL = parts[i].val?.value;
      let valueR = parts[i+2]?.val?.value;
      // if both sides are (unswizzled) numbers, we can evaluate them
      if(parts[i].swizzle || parts[i+2]?.swizzle)continue;
      if(typeof(valueL) === "number" && typeof(valueR) === "number")
      {
        let newVal = undefined;
        switch(parts[i+1])
        {
          case '+': newVal = valueL + valueR; break;
          case '-': newVal = valueL - valueR; break;
          case '*': newVal = valueL * valueR; break;
          case '/': newVal = valueL / valueR; break;
          case '<<': newVal = valueL << valueR; break;
          case '>>': newVal = valueL >> valueR; break;
          case '>>>': newVal = valueL >>> valueR; break;
          case '&': newVal = valueL & valueR; break;
          case '^': newVal = valueL ^ valueR; break;
          case '|': newVal = valueL | valueR; break;
          default: state.throwError("Unsupported operation for compile-time evaluation!", valueL + parts[i+1] + valueR);
        }
        //console.log("eval", valueL, parts[i+1], valueR, newVal);

        // if the operator can be handled and we got a result, replace the 3 parts with the value
        if(newVal !== undefined) {
          parts.splice(i, 3, {val: {type: "num", value: newVal}, swizzle: undefined});
          i--;
        }
      }
    }
  }

  return parts.length === 1 ? parts[0] : parts;
}

function printParts(parts) {
  let str = "";
  for(const part of parts) {
    if(typeof(part) === "string") {
      str += part;
    } else if(Array.isArray(part)) {
      str += "(" + printParts(part) + ")";
    } else {
      str += part.val.value;
    }
  }
  return str;
}

/**
 *
 * @param {ASTAssignCalc} st
 * @param {ASTState[]} astState
 * @return {*[]}
 */
export function astCalcNormalize(st, astState)
{
  const newStm = [];
  if(st.assignType !== "=") {
    state.throwError("@TODO: multiple calculation in an increment not supported!", st);
  }

  // First flatten out the linked-list structure into an array of operators, brackets, and vars
  let parts = [];
  for(let i=0; i<st.calc.groupStart; ++i)parts.push("(");
  parts.push({val: st.calc.left, swizzle: st.calc.swizzleLeft});

  for(const part of st.calc.parts) {
    parts.push(part.op);
    for(let i=0; i<part.groupStart; ++i)parts.push("(");
    parts.push({val: part.right, swizzle: part.swizzleRight});
    for(let i=0; i<part.groupEnd; ++i)parts.push(")");
  }

  //console.log(parts);
  parts = partsToTree(parts)
  parts = applyPrecedence(parts);
  parts = partsEval(parts);
  if(!Array.isArray(parts))parts = [parts];
  //console.log("EVAL" , printParts(parts));

  if(parts.length === 1) {
    newStm.push({
      type: "varAssignCalc",
      varName: st.varName,
      calc:  {
        type: "calcNum",
        right: parts[0].val,
      },
      assignType: "=",
      line: st.line,
    })
    return newStm;
  }

  let lastLeft = st.calc.left;
  let lastLeftSwizzle = st.swizzle;

  // now turn into actual individual assignments
  let firstPart = parts[0];
  for(let i=1; i<parts.length; i+=2) {
    const op = parts[i];
    const part = parts[i+1];
    if(Array.isArray(part)) {
      console.log(printParts(parts));
      state.throwError("@TODO: Nested brackets not supported!", part);
    }

    /** @type {ASTCalcLR} */
    const calcLR = {
      type: "calcLR",
      right: part.val,
      op,
      swizzleRight: part.swizzle,
      left: lastLeft,
      swizzleLeft: lastLeftSwizzle
    };

    const stateVar = astState.find(s => s.varName === part.val.value);
    if(stateVar) {
      //calcLR.type = "calcNum";
      calcLR.right = {type: 'num', value: `%lo(${part.val.value})`};
    }

    newStm.push({
      type: "varAssignCalc",
      varName: st.varName,
      calc: calcLR,
      assignType: "=",
      line: st.line,
    });
    // now use result as left side
    lastLeft = {type: 'VarName', value: st.varName};
    lastLeftSwizzle = st.swizzle;
  }
  return newStm;
}
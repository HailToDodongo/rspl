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
  // First flatten out the linked-list structure into an array of operators, brackets, and vars
  let parts = [];

  if(st.assignType !== "=") {
    const expOp = st.assignType.substring(0, st.assignType.length-1);
    parts.push({val: {type: 'VarName', value: st.varName}, swizzle: st.swizzle});
    parts.push(expOp);
    parts.push("(");
  }

  for(let i=0; i<st.calc.groupStart; ++i)parts.push("(");
  parts.push({val: st.calc.left, swizzle: st.calc.swizzleLeft});

  for(const part of st.calc.parts) {
    parts.push(part.op);
    for(let i=0; i<part.groupStart; ++i)parts.push("(");
    parts.push({val: part.right, swizzle: part.swizzleRight});
    for(let i=0; i<part.groupEnd; ++i)parts.push(")");
  }

  if(st.assignType !== "=") {
    parts.push(")");
    st.assignType = "=";
    st.calc.type = 'calcLR';
  }

  parts = partsToTree(parts)
  return applyPrecedence(parts);
}

/**
 * Converts a normalized AST calculation into ASM.
 * This step is deferred from the normalization stage into actual ASM generation,
 * this is done to allow for temp. regs to be used on a per-case basis, as well
 * as cont-eval with values per use-case.
 *
 * @param st {ASTNestedCalc}
 * @return {ASTStatement[]}
 */
export function astCalcPartsToASM(st)
{
  let parts = structuredClone(st.parts);
  parts = partsEval(parts);
  if(!Array.isArray(parts))parts = [parts];

  if(parts.length === 1) {
    return [{
      type: "varAssignCalc",
      varName: st.varName,
      calc:  {
        type: "calcNum",
        right: parts[0].val,
      },
      assignType: "=",
      line: st.line,
    }];
  }

  let tmpVarBaseName = "__tmp" + st.line + "_";
  let tmpVarStack = [];
  const targetType = state.getRequiredVar(st.varName, "syntax expansion").type;

  /**
   * Flattens out an array of statement into a single variable assignment.
   * This will insert all calculations into "newStm" and returns a new part
   * that will contain the target variable of all those statements.
   *
   * @param {ASTStatement[]} newStm
   * @param {ASTCalcMultiPart[]} parts
   * @param {string} targetVar
   * @param {Swizzle} targetSwizzle
   * @return {{newStm: ASTStatement[], part: ASTCalcMultiPart}}
   */
  const flattenPartArray = (parts, targetSwizzle) => {
    const tmpVarName = tmpVarBaseName + tmpVarStack.length;
    tmpVarStack.push(tmpVarName);

    return {
      newStm: [{
          type: 'varDecl',
          varName: tmpVarName,
          reg: undefined,
          varType: targetType,
          isConst: false,
        },
        ...partsToAsm(parts, tmpVarName, undefined)
      ],
      part: {
        val: {type: 'VarName', value: tmpVarName},
        swizzle: targetSwizzle,
      }
    };
  };

  /**
   * Converts a list of (potentially nested) parts into ASM statements.
   * @param {ASTCalcMultiPart[]} parts
   * @param targetVar
   * @param targetSwizzle
   * @return {ASTStatement[]}
   */
  const partsToAsm = (parts, targetVar, targetSwizzle) => {
    const newStm = [];
    if(Array.isArray(parts[0])) {
      const res = flattenPartArray(parts[0], targetSwizzle);
      parts[0] = res.part;
      newStm.push(...res.newStm);
    }

    let lastLeft = parts[0].val;
    let lastLeftSwizzle = parts[0].swizzle;

    // now turn into actual individual assignments
    //console.log("EVAL" , printParts(parts), parts[0]);

    for(let i=1; i<parts.length; i+=2) {
      const op = parts[i];
      let part = parts[i+1];

      if(Array.isArray(part)) {
        const res = flattenPartArray(part, targetSwizzle);
        part = res.part;
        newStm.push(...res.newStm);
      }

      const stateVar = state.memVarMap[part.val.value];

      newStm.push({
        type: "varAssignCalc",
        varName: targetVar,
        calc: {
          type: "calcLR", op,
          left: lastLeft,
          swizzleLeft: lastLeftSwizzle,
          right: stateVar ? {type: 'num', value: `%lo(${part.val.value})`} : part.val,
          swizzleRight: part.swizzle,
        },
        assignType: "=",
        line: st.line,
      });

      // now use result as left side
      lastLeft = {type: 'VarName', value: targetVar};
      lastLeftSwizzle = targetSwizzle;
    }

    return newStm;
  };

  return [
    ...partsToAsm(parts, st.varName, st.swizzle),
    ...tmpVarStack.map(varName => ({type: 'varUndef', varName}))
  ];
}
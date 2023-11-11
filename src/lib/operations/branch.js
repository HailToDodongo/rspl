import {REG} from "../syntax/registers.js";
import state from "../state.js";
import {asm, asmNOP} from "../intsructions/asmWriter.js";
import {isSigned, u32InS16Range} from "../dataTypes/dataTypes.js";
import opsScalar from "./scalar.js";

/**
 * Creates a branch instruction with a comparison against a register or immediate.
 * Immediate values are loaded into a register first if necessary.
 * This will only jump if the comparison fails, so the "true" case is expected to follow the branch.
 *
 * @param {ASTCompare} compare operation and left/right values
 * @param labelElse label to jump to if the comparison fails (aka "else")
 * @returns {ASM[]} list of ASM instructions
 */
export function opBranch(compare, labelElse)
{
  compare = structuredClone(compare); // gets modified

  let isImmediate = compare.right.type === "num";
  let regTestRes = isImmediate ? REG.AT
    : state.getRequiredVar(compare.right.value, "compare").reg;

  // use register for zero-checks (avoids potential imm. load instructions)
  if(isImmediate && compare.right.value === 0) {
    isImmediate = false;
    regTestRes = REG.ZERO;
  }

  let {reg: regLeft, type: baseType} = state.getRequiredVar(compare.left.value, "left");

  // Easy case, just compare
  if(compare.op === "==" || compare.op === "!=")
  {
    const opBranch = compare.op === "==" ? "bne" : "beq";
    return [
      ...(isImmediate ? opsScalar.loadImmediate(REG.AT, compare.right.value) : []),
      asm(opBranch, [regLeft, regTestRes, labelElse]),
      asmNOP(),
    ];
  }

  const opsLoad = [];
  let regOrValRight = isImmediate ? compare.right.value : regTestRes;

  // Both ">" and "<=" are causing the biggest issues when inverted, so map them to the other two
  if(compare.op === ">" || compare.op === "<=") {
    if(isImmediate) {
      // add one to the immediate to add/remove the "=" part of the comparison.
      // Ignore overflows here (e.g. "x > 0xFFFFFFFF" would be stupid anyway)
      regOrValRight = (regOrValRight+1) >>> 0;
      compare.op = compare.op === ">" ? ">=" : "<";
    } else {
      compare.op = compare.op === ">" ? "<" : ">="; // invert comparison ...
      [regLeft, regOrValRight] = [regOrValRight, regLeft]; //... and swap registers
    }
  }

  // All values from now on need signedness checks, first check if it can still be an immediate
  if(isImmediate && !u32InS16Range(regOrValRight >>> 0)) {
    // ...if it doesn't we load it and switch back to a register check
    opsLoad.push(...opsScalar.loadImmediate(REG.AT, regOrValRight));
    isImmediate = false;
    regOrValRight = REG.AT;
  }

  const signed = isSigned(baseType); // Note: both the signed/unsigned 'slt' have a sign-extended immediate
  const opLessThan = "slt" + (isImmediate ? "i" : "") + (signed ? "" : "u");

  if(compare.op === "<" || compare.op === ">=")
  {
    const opBranch = compare.op === "<" ? "beq" : "bne";
    return [
      ...opsLoad,
      asm(opLessThan, [REG.AT, regLeft, regOrValRight]),
      asm(opBranch, [REG.AT, REG.ZERO, labelElse]), // jump if "<" fails (aka ">=")
      asmNOP(),
    ];
  }

  return state.throwError("Unknown comparison operator: " + compare.op, compare);
}
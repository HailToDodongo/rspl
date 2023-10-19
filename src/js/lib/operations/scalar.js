/**
* @copyright 2023 - Max Bebök
* @license GPL-3.0
*/
import state from "../state";

const isSigned = (type) => type.startsWith("s");

function opAdd(varRes, varLeft, varRight)
{
  if(isSigned(varRes.type)) {
    return [["add", varRes.reg, varLeft.reg, varRight.reg]];
  } else {
    return [["addu", varRes.reg, varLeft.reg, varRight.reg]];
  }
}

function opMul(varRes, varLeft, varRight)
{
  state.throwError("Scalar-Multiplication not implemented!");
}

export default {opAdd, opMul};
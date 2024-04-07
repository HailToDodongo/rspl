/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/
import {asm} from "../../intsructions/asmWriter.js";
import {REG} from "../../syntax/registers.js";
import state from "../../state.js";

/**
 * If a branch has the form of: " if(cond)goto TARGET; "
 * It will result in:
 *   beq ... TEMP_LABEL  // inverted by default
 *   nop
 *   j TARGET
 *   nop
 *   TEMP_LABEL:
 *   ...
 *
 * We can invert the condition to avoid to jumps and a label.
 *
 * If the `j TARGET` is a `jal TARGET` instead, we can still apply this optimization.
 * However, a manual assignment of the return register is required.#
 *
 * @param {ASMFunc} asmFunc
 */
export function branchJump(asmFunc)
{
  const lines = asmFunc.asm;
  for(let i=0; i<lines.length; ++i)
  {
    const line = lines[i];
    if(line.op === "beq" || line.op === "bne")
    {
      const labelTemp = line.args[line.args.length-1];
      const jumpOp = lines[i+2]?.op;

      if(lines[i+1]?.isNOP &&
          (jumpOp === "j" || jumpOp === "jal")   &&
         lines[i+3]?.isNOP &&
         lines[i+4]?.label === labelTemp)
      {
        const labelTarget = lines[i+2].args[lines[i+2].args.length-1];
        // invert condition and patch label
        line.op = line.op === "beq" ? "bne" : "beq";
        line.args[line.args.length-1] = labelTarget;

        // if it was a jal, we need to manually assign the return register
        if(jumpOp === "jal") {
          lines.splice(i+2, 2); // remove jump, delay-slot

          if(!line.labelEnd)state.throwError("Missing labelEnd for branch, this is a bug in RSPL, please let me know");
          lines.splice(i, 0, asm("ori", [REG.RA, REG.ZERO, line.labelEnd]));
        } else {
          lines.splice(i+2, 3); // remove jump, delay-slot, temp-label
        }

        i-=2;
      }
    }

  }
}
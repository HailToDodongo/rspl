/**
* @copyright 2024 - Max Bebök
* @license Apache-2.0
*/

const KNOWN_END_JUMPS = ["RSPQ_Loop"];

/**
 * If a return to the main loop happens right after a function call,
 * we can instead do a jump instead of a jump+link, removing the need for a return.
 *
 * E.g.
 *   jal DMAExec; nop;
 *   j RSPQ_Loop; nop;
 *
 * Can be turned into:
 *   j DMAExec; nop;
 *
 * @param {ASMFunc} asmFunc
 */
export function tailCall(asmFunc)
{
  if(asmFunc.type !== "command")return; // only works when one level deep
  const lines = asmFunc.asm;
  for(let i=0; i<lines.length; ++i)
  {
    const asm = lines[i];
    if(asm.op === "jal")
    {
      if(lines[i+1]?.isNOP &&
         lines[i+2]?.op === "j" &&
         KNOWN_END_JUMPS.includes(lines[i+2]?.args[0]) &&
         lines[i+3]?.isNOP)
      {
        asm.op = "j"; // turn into jump to let the return inside replace our next jump
        lines.splice(i+1, 2); // remove the jump and the nop
        i-=2;
      }

      // if we encounter a jump, but the above condition is not met, we can stop
      // otherwise it would mean that the return register was changed.
      return;
    }

  }
}
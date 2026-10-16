/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {isGeneratedLabel} from "../labels.js";

/**
 * De-duplicates labels.
 * Two adjacent labels may only be dropped if they are compiler generated
 * @param {ASMFunc} asmFunc
 */
export function dedupeLabels(asmFunc)
{
  const lines = asmFunc.asm;
  for(let i=0; i+1 < lines.length; ++i)
  {
    const a = lines[i], b = lines[i+1];
    if(a.type !== ASM_TYPE.LABEL || b.type !== ASM_TYPE.LABEL)continue;
    if(a.label.startsWith("__") || b.label.startsWith("__"))continue;

    let drop;
    if(isGeneratedLabel(asmFunc, a.label))drop = i;
    else if(isGeneratedLabel(asmFunc, b.label))drop = i+1;
    else continue;

    const from = lines[drop].label;
    const to = lines[drop === i ? i+1 : i].label;
    for(const asm of lines) {
      if(asm.labelEnd === from)asm.labelEnd = to;
      if(asm.args) {
        for(let k=0; k<asm.args.length; ++k) {
          if(asm.args[k] === from)asm.args[k] = to;
        }
      }
    }
    lines.splice(drop, 1);
    --i;
  }
}

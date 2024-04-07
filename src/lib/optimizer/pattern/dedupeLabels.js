/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

import {ASM_TYPE} from "../../intsructions/asmWriter.js";
import {BRANCH_OPS} from "../asmScanDeps.js";

/**
 * De-duplicates labels.
 * @param {ASMFunc} asmFunc
 */
export function dedupeLabels(asmFunc)
{
  // de-duplicate labels, first detect consecutive labels...
  const labelsDelete = [];
  const labelsReplace = {};
  let labels = [];
  for(const asm of asmFunc.asm)
  {
    if(asm.type === ASM_TYPE.LABEL) {
      labels.push(asm.label);
    } else {
      if(labels.length > 1) {
        const newLabel = labels.pop();
        labelsDelete.push(...labels);
        for(const label of labels)labelsReplace[label] = [newLabel];
      }
      labels = [];
    }
  }

  // ...now keep the first one, remove the others and patch the references
  const asmNew = [];
  for(const asm of asmFunc.asm)
  {
    if(labelsDelete.includes(asm.label))continue;
    if(BRANCH_OPS.includes(asm.op)) {
      const label = asm.args[asm.args.length-1];
      if(labelsReplace[label]) {
        asm.args[asm.args.length-1] = labelsReplace[label][0];
      }
    }
    asmNew.push(asm);
  }
  asmFunc.asm = asmNew;
}
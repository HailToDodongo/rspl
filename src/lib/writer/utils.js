/**
* @copyright 2023 - Max Bebök
* @license Apache-2.0
*/

export function getAttrLoaderLabel(asm) {
  if (!asm.attrLoader) return null;
  return "LOAD_" + asm.attrLoader + asm.debug.lineRSPL;
}

export function getAttrPatchLabel(asm) {
  if (!asm.attrPatch?.name) return null;
  return "PATCH_" + asm.attrPatch.name + asm.debug.lineRSPL;
}

export function getAsmLabels(asm) {
  let labels = "";
  for(const label of [getAttrLoaderLabel(asm), getAttrPatchLabel(asm)].filter(l => l)) {
    labels += label + ": ";
  }
  return labels;
}

/**
 * @param {ASM} asm
 * @returns {string}
 */
export function stringifyInstr(asm) {
  return getAsmLabels(asm) + asm.op + (asm.args.length ? (" " + asm.args.join(", ")) : "");
}
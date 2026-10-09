/**
* @copyright 2026 - Max Bebök
* @license Apache-2.0
*/

/**
 * True for a label the compiler generated
 * @param {ASMFunc} asmFunc
 * @param {string} label
 * @return {boolean}
 */
export function isGeneratedLabel(asmFunc, label)
{
  const prefix = `LABEL_${asmFunc.name}_`;
  if(!label || label.length !== prefix.length + 4 || !label.startsWith(prefix))return false;
  return /^[0-9A-F]{4}$/.test(label.substring(prefix.length));
}

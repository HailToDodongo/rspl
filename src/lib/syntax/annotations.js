/**
* @copyright 2024 - Max Bebök
* @license Apache-2.0
*/
import state from "../state.js";

export const KNOWN_ANNOTATIONS = [
  "Barrier"
];

/**
 * Checks if an annotation is known and valid.
 * @param {Annotation} anno
 * @throws {Error} if invalid
 */
export function validateAnnotation(anno) {
  if(!KNOWN_ANNOTATIONS.includes(anno.name)) {
    state.throwError("Unknown annotation '"+anno.name+"'!\nExpected on of: "+KNOWN_ANNOTATIONS.join(", ")+"");
  }

  // string annotations
  if(["Barrier"].includes(anno.name)) {
    if(typeof anno.value !== "string") {
      state.throwError("Annotation '"+anno.name+"' expects a string value!");
    }
  }
}
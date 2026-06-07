/**
* @copyright 2024 - Max Bebök
* @license Apache-2.0
*/
import state from "../state.js";

export const ANNOTATIONS = {
  Barrier: "Barrier",
  Relative: "Relative",
  Align: "Align",
  NoReturn: "NoReturn",
  Unlikely: "Unlikely",
  NoRegAlloc: "NoRegAlloc",
  Tag: "Tag",
  AttrLoader: "AttrLoader",
  AttrPatch: "AttrPatch"
};

export const KNOWN_ANNOTATIONS = Object.keys(ANNOTATIONS);

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
  if(["Barrier", "AttrLoader", "AttrPatch"].includes(anno.name)) {
    if(typeof anno.value !== "string") {
      state.throwError("Annotation '"+anno.name+"' expects a string value!");
    }
    if(anno.value === "") {
      state.throwError("Annotation '"+anno.name+"' expects a non-empty string value!");
    }
  }
}

export function getAnnotationVal(annotations, name) {
  const anno = annotations.find(anno => anno.name === name);
  return anno ? (anno.value || true) : undefined;
}

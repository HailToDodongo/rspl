#include "annotations.h"

#include "state.h"

#include <algorithm>

namespace rspl {

const std::vector<std::string> KNOWN_ANNOTATIONS = {
    "Barrier", "Relative", "Align",      "NoReturn",
    "Unlikely", "NoRegAlloc", "Tag",     "AttrLoader",
    "AttrPatch"};

// Annotations that must carry a non-empty string value.
static const std::vector<std::string> STRING_ANNOTATIONS = {
    "Barrier", "AttrLoader", "AttrPatch"};

void validateAnnotation(const std::string &name, const std::string &value,
                        bool valueIsString) {
  if (std::find(KNOWN_ANNOTATIONS.begin(), KNOWN_ANNOTATIONS.end(), name) ==
      KNOWN_ANNOTATIONS.end()) {
    std::string known;
    for (size_t i = 0; i < KNOWN_ANNOTATIONS.size(); ++i) {
      if (i) known += ", ";
      known += KNOWN_ANNOTATIONS[i];
    }
    state.throwError("Unknown annotation '" + name + "'!\nExpected on of: " +
                     known);
  }

  if (std::find(STRING_ANNOTATIONS.begin(), STRING_ANNOTATIONS.end(), name) !=
      STRING_ANNOTATIONS.end()) {
    if (!valueIsString) {
      state.throwError("Annotation '" + name + "' expects a string value!");
    }
    if (value.empty()) {
      state.throwError("Annotation '" + name +
                       "' expects a non-empty string value!");
    }
  }
}

} // namespace rspl

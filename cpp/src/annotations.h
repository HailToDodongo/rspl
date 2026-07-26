#pragma once

#include <string>
#include <vector>

namespace rspl {

// Every annotation the transpiler understands.
extern const std::vector<std::string> KNOWN_ANNOTATIONS;

/// Throws if the annotation is unknown or carries an invalid value.
void validateAnnotation(const std::string &name, const std::string &value,
                        bool valueIsString);

} // namespace rspl

#pragma once

#include "ast.h"

namespace rspl {

/// Validates the program and fills in values the parser leaves open
/// (currently uniform / vertex-attribute binding numbers).
/// Mirrors the JS astNormalizeState/Uniforms/Attributes/Functions passes.
/// Throws on invalid programs.
void astNormalize(ast::Program &prog, bool magma);

} // namespace rspl

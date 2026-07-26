#include "ast_normalize.h"

#include "annotations.h"
#include "state.h"

#include <set>

namespace rspl {

namespace {

constexpr int64_t BINDING_LIMIT = int64_t{1} << 32;

// Assigns binding numbers to entries that did not specify one, skipping
// numbers already taken by explicit bindings. Mirrors the shared logic of
// astNormalizeUniforms / astNormalizeAttributes.
template <typename T>
void assignBindings(std::vector<T> &entries, bool magma,
                    const char *notMagmaError, const char *rangeError) {
  std::set<int64_t> used;
  for (const auto &e : entries) {
    if (e.binding.has_value()) used.insert(*e.binding);
  }

  int64_t current = 0;
  for (auto &e : entries) {
    state.line = e.line;
    if (!magma) state.throwError(notMagmaError);

    if (e.binding.has_value()) {
      if (*e.binding < 0 || *e.binding >= BINDING_LIMIT) {
        state.throwError(rangeError);
      }
      current = *e.binding;
    } else {
      while (used.count(current)) ++current;
      e.binding = current;
    }
    used.insert(current);
  }
}

} // namespace

void astNormalize(ast::Program &prog, bool magma) {
  state.func = "state";
  state.line = 0;

  // --- State sections: magma shaders keep all their data in uniforms ---
  if (magma) {
    for (const auto &sec : prog.states) {
      for (const auto &sv : sec.vars) {
        if (!sv.isExtern) {
          state.throwError(
              "Only extern states are allowed when compiling for magma!");
        }
      }
    }
  }

  assignBindings(prog.uniforms, magma,
                 "Uniforms are only allowed when compiling for magma "
                 "(pass '--magma' on the command line)!",
                 "Uniform binding number must be in [0, 2^32)!");

  assignBindings(prog.attributes, magma,
                 "Attributes are only allowed when compiling for magma "
                 "(pass '--magma' on the command line)!",
                 "Attribute input number must be in [0, 2^32)!");

  // --- Functions -------------------------------------------------------
  int shaderCount = 0;
  for (const auto &fn : prog.functions) {
    if (!fn.body) continue;

    state.func = fn.name;
    state.line = fn.body->line;

    for (const auto &anno : fn.annotations) {
      validateAnnotation(anno.name, anno.value, anno.valueIsString);
    }

    if (fn.type == FuncType::Command) {
      if (magma) {
        state.throwError("Commands must not be defined when compiling for "
                         "magma (define a 'shader' instead)!");
      }
      if (!fn.hasResultType) {
        state.throwError(
            "Commands must specify an index (e.g. 'command<4>')!");
      }
    }

    if (fn.type == FuncType::Macro && fn.hasResultType) {
      state.throwError("Macros must not specify a result-type (use 'macro' "
                       "without `< >`)!");
    }

    if (fn.type == FuncType::Shader) {
      if (!magma) {
        state.throwError("Shaders are only allowed when compiling for magma "
                         "(pass '--magma' on the command line)!");
      }
      if (shaderCount > 0) {
        state.throwError("A shader has already been defined!");
      }
      if (fn.hasResultType) {
        state.throwError("Shaders must not specify a result-type (use "
                         "'shader' without `< >`)!");
      }
      if (!fn.args.empty()) {
        state.throwError("Shaders must not specify arguments!");
      }
      ++shaderCount;
    }
  }

  if (magma && shaderCount == 0) {
    state.func = "";
    state.line = 0;
    state.throwError("Exactly one shader must be defined when compiling for "
                     "magma (use 'shader')!");
  }
}

} // namespace rspl

#pragma once

#include "../asm.h"

namespace rspl {

/// Run pattern-based optimizations (dedupe labels, dedupe jumps, etc.)
void asmOptimizePattern(AsmFunc &func);

/// Fill NOP delay slots by moving independent instructions forward.
/// Must be called after asmScanDeps.
void fillDelaySlots(AsmFunc &func);

/// Run reorder optimization (stochastic annealing) on a single function.
/// updateCb is called whenever a better result is found.
void asmOptimize(AsmFunc &func, int maxTimeMs = 30'000);

} // namespace rspl

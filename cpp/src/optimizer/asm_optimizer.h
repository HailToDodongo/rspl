#pragma once

#include "../asm.h"

namespace rspl {

/// Run pattern-based optimizations (dedupe labels, dedupe jumps, etc.)
void asmOptimizePattern(AsmFunc &func);

/// Fill NOP delay slots by moving independent instructions forward.
/// Must be called after asmScanDeps.
void fillDelaySlots(AsmFunc &func);

/// Set the PRNG seed for reproducible reorder results (used by tests).
void setSeed(uint32_t s);

/// Run reorder optimization (stochastic annealing) on a single function.
/// optWorkers: 0 = auto-detect, otherwise that many threads.
void asmOptimize(AsmFunc &func, int maxTimeMs = 30'000, int optWorkers = 0);

/// Print cumulative reorder stats (total iterations, average IPS)
/// across all asmOptimize calls since program start.
void printCumulativeStats();

} // namespace rspl
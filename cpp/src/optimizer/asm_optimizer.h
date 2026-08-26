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
/// optSeed: 0 = seed from system entropy, otherwise a fixed base seed.
/// optIters: 0 = stop on maxTimeMs wall time, otherwise run exactly that
///           many meta-iterations (deterministic together with optSeed;
///           pin optWorkers too when comparing across machines, since the
///           batch size scales with the worker count).
void asmOptimize(AsmFunc &func, int maxTimeMs = 30'000, int optWorkers = 0,
                 uint32_t optSeed = 0, int optIters = 0);

/// Print cumulative reorder stats (total iterations, average IPS)
/// across all asmOptimize calls since program start.
void printCumulativeStats();

} // namespace rspl
#pragma once

#include "../asm.h"

namespace rspl {

/// Run pattern-based optimizations (dedupe labels, dedupe jumps, etc.)
void asmOptimizePattern(AsmFunc &func);

/// True for a label the compiler generated for this function
/// (State::generateLabel: "LABEL_<func>_XXXX"). Only those may be removed
/// or renamed by a pattern pass: a user-written label can be referenced
/// from other functions or from hand-written assembly the pass cannot see.
inline bool isGeneratedLabel(const AsmFunc &func, const std::string &label) {
  const std::string prefix = "LABEL_" + func.name + "_";
  if(label.size() != prefix.size() + 4 || label.compare(0, prefix.size(), prefix) != 0) return false;
  for(size_t i = prefix.size(); i < label.size(); ++i)
  {
    char c = label[i];
    if(!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}

/// Fill NOP delay slots by moving independent instructions forward.
/// Must be called after asmScanDeps.
void fillDelaySlots(AsmFunc &func);

/// Set the PRNG seed for reproducible reorder results (used by tests).
void setSeed(uint32_t s);

/// Run reorder optimization (stochastic annealing) on a single function.
/// optWorkers: 0 = auto-detect, otherwise that many threads.
/// optSeed: 0 = seed from system entropy, otherwise a fixed base seed.
/// optAnneal: simulated-annealing acceptance (a worse variant may become
///   the current state with probability exp(-delta/T), T cooling over the
///   budget); the best state seen is what gets emitted. Lets the search cross
///   worse intermediate orders (e.g. shifting a long run by one position to
///   flip its pairing phase), which plain hill-climbing never does.
/// optIters: 0 = stop on maxTimeMs wall time, otherwise run exactly that
///           many meta-iterations (deterministic together with optSeed;
///           pin optWorkers too when comparing across machines, since the
///           batch size scales with the worker count).
void asmOptimize(AsmFunc &func, int maxTimeMs = 30'000, int optWorkers = 0, uint32_t optSeed = 0, int optIters = 0,
                 bool optAnneal = false);

/// Print cumulative reorder stats (total iterations, average IPS)
/// across all asmOptimize calls since program start.
void printCumulativeStats();

} // namespace rspl
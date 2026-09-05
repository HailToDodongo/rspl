#include "asm_optimizer.h"
#include "asm.h"
#include "asm_scan_deps.h"
#include "compact_sched.h"
#include "eval_cost.h"
#include "patterns/assertCompare.h"
#include "patterns/branchJump.h"
#include "patterns/commandAlias.h"
#include "patterns/dedupeImm.h"
#include "patterns/dedupeJumps.h"
#include "patterns/dedupeLabels.h"
#include "patterns/mergeSequence.h"
#include "patterns/removeDeadCode.h"
#include "patterns/tailCall.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

namespace rspl {

// --- PRNG (LCG, one stream per worker thread) ------------------------------

static thread_local uint32_t seed_ = 0x41C64E6D;

void setSeed(uint32_t s) { seed_ = s; }

static double rand01() {
  seed_ = (seed_ * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
  return (seed_ >> 16) / 65536.0;
}

static int randIndex(int maxExcl) {
  seed_ = (seed_ * 0x41C64E6D + 0x3039) & 0xFFFFFFFF;
  return (seed_ >> 16) % maxExcl;
}

// splitmix32: derive independent sub-seeds from (base, index) pairs. 
// makes sure threads get the same seed no matter the order
static uint32_t mixSeed(uint32_t a, uint32_t b) {
  uint32_t z = a + 0x9E3779B9u * (b + 1);
  z = (z ^ (z >> 16)) * 0x21F0AAADu;
  z = (z ^ (z >> 15)) * 0x735A2D97u;
  return z ^ (z >> 15);
}

// --- Pattern optimization runner --------------------------------------

void asmOptimizePattern(AsmFunc &func) {
  dedupeLabels(func);
  dedupeJumps(func);
  branchJump(func);
  tailCall(func);
  dedupeImmediate(func);
  mergeSequence(func);
  assertCompare(func);
  removeDeadCode(func);
  commandAlias(func);
}

// --- Delay slot filling -----------------------------------------------

void fillDelaySlots(AsmFunc &func) {
  CompactFunc cf = compactBuild(func);
  CompactState st = compactInitialState(cf);
  std::vector<int> range;
  for (int i = 0; i < (int)st.seq.size(); ++i) {
    const CompactOp &op = cf.ops[st.seq[i]];
    if (!op.isOp || (op.flags & (OpFlag::OP_FLAG_IS_IMMOVABLE |
                                 OpFlag::OP_FLAG_IS_NOP |
                                 OpFlag::OP_FLAG_IS_BRANCH)))
      continue;
    compactReorderIndices(cf, st, i, range);
    for (int idx : range) {
      if (idx > i && cf.ops[st.seq[idx]].isNop) {
        compactRelocate(cf, st, i, idx); // fills the slot, drops the NOP
        --i;
        break;
      }
    }
  }
  compactApply(cf, st, func);
}

// ==========================================================================
// Reorder optimization (stochastic search over instruction orders)
// ==========================================================================

// --- Constants -------------------------------------------------------------

constexpr int POOL_SIZE = 8;
constexpr double PREFER_STALLS_RATE = 0.20;
constexpr double PREFER_PAIR_RATE = 0.80;
// Chance per step to attempt an offset-rebase hop (mem-op crossing a pointer
// increment, see asmTryRebaseCross). Disable with RSPL_REBASE_HOP=0.
constexpr double REBASE_HOP_RATE = 0.10;
constexpr int MAX_STEPS_NO_CHANGE = 5000;
constexpr int SEARCH_VARIANT_SEARCH = 10;
constexpr int SEARCH_BACK_STEPS_FACTOR = 10;
constexpr int SEARCH_FWD_STEPS_FACTOR = 5;
constexpr int REORDER_MIN_OPS = 3;
constexpr int REORDER_MAX_OPS = 15;
constexpr int PROGRESS_LOG_INTERVAL = 500; // meta-iterations between logs

// --- Helper functions -----------------------------------------------------

// ============================================================================
// Reorder search inner loop on the compact representation (see
// compact_sched.h): a variant is just an item order, evaluated with the
// same rules as evalFunctionCost (equality is unit-tested).
// ============================================================================

static int countOpsSeq(const CompactFunc &cf, const CompactState &st) {
  int count = 0;
  for (int id : st.seq) if (cf.ops[id].isOp) ++count;
  return count;
}

static bool rebaseHopEnabled() {
  static const bool enabled = [] {
    const char *e = std::getenv("RSPL_REBASE_HOP");
    return !(e && e[0] == '0');
  }();
  return enabled;
}

// --- optimizeStep: one random legal move ------------------------------------

static int optimizeStep(const CompactFunc &cf, CompactState &st) {
  auto sz = static_cast<int>(st.seq.size());
  if (sz < 2) return 0;

  // Occasionally try an offset-rebase hop; most picks are not rebasable
  // mem-ops and fall through to the normal move below at trivial cost.
  if (rebaseHopEnabled() && rand01() < REBASE_HOP_RATE) {
    int hopIdx = randIndex(sz);
    bool fwd = rand01() < 0.5;
    if (compactTryRebaseCross(cf, st, hopIdx, fwd) ||
        compactTryRebaseCross(cf, st, hopIdx, !fwd)) {
      return 1;
    }
  }

  int i = 0;
  static thread_local std::vector<int> reorderIndices;
  for (int r = 0; r < 50; ++r) {
    i = randIndex(sz);
    compactReorderIndices(cf, st, i, reorderIndices);
    if ((int)reorderIndices.size() > 1) break;
  }
  if ((int)reorderIndices.size() <= 1) return 0;

  const CompactOp &opI = cf.ops[st.seq[i]];
  int targetIdx = i;
  bool foundIndex = false;

  // Prefer pairing opposite-type (vector<->scalar) unpaired instructions
  if (rand01() < PREFER_PAIR_RATE) {
    for (int j : reorderIndices) {
      const CompactOp &opJ = cf.ops[st.seq[j]];
      if ((opJ.flags & OpFlag::OP_FLAG_IS_VECTOR) != (opI.flags & OpFlag::OP_FLAG_IS_VECTOR)) {
        if (!st.paired[st.seq[j]]) {
          targetIdx = j;
          foundIndex = true;
        }
      }
    }
    if (!foundIndex) return 0;
  }

  // Prefer filling high-stall positions
  if (!foundIndex && rand01() < PREFER_STALLS_RATE) {
    int maxStalls = 0;
    for (int j : reorderIndices) {
      int stalls = st.stall[st.seq[j]];
      if (stalls > maxStalls) {
        maxStalls = stalls;
        targetIdx = j;
        foundIndex = true;
      }
    }
  }

  if (!foundIndex) {
    while (targetIdx == i) {
      targetIdx = reorderIndices[randIndex((int)reorderIndices.size())];
    }
  }

  compactRelocate(cf, st, i, targetIdx);
  return 1;
}

// --- reorderRound: a few moves, then evaluate --------------------------------

struct RoundResult {
  int cost = 0;
  CompactState st;
};

// Writes into `r` (copy-assignment reuses its vectors' capacity, so a
// persistent RoundResult makes a round allocation-free).
static void reorderRound(const CompactFunc &cf, const CompactState &base,
                         RoundResult &r) {
  r.st = base;
  int opCount = randIndex(REORDER_MAX_OPS - REORDER_MIN_OPS) + REORDER_MIN_OPS;
  for (int o = 0; o < opCount; ++o) optimizeStep(cf, r.st);
  r.cost = compactEvalCost(cf, r.st);
}

// --- generateWorseFunction: escape a local minimum ----------------------------

static std::pair<CompactState, int> generateWorseFunction(const CompactFunc &cf,
                                                          const CompactState &base,
                                                          int steps) {
  int maxCost = 0;
  CompactState newWorst = base;
  static thread_local RoundResult a, b;
  for (int i = 0; i < steps; ++i) {
    reorderRound(cf, base, a);
    reorderRound(cf, a.st, b);
    if (b.cost > maxCost) {
      newWorst = b.st;
      maxCost = b.cost;
    }
  }
  return {std::move(newWorst), maxCost};
}

// --- Timing helpers ----------------------------------------------------

static std::string formatTimeMs(int ms) {
  std::ostringstream oss;
  if (ms >= 1000) {
    oss << std::fixed << std::setprecision(1) << (ms / 1000.0) << "s";
  } else {
    oss << ms << "ms";
  }
  return oss.str();
}

// --- Worker pool -------------------------------------------------------

class WorkerPool {
public:
  explicit WorkerPool(int numWorkers) {
    for (int i = 0; i < numWorkers; ++i)
      threads_.emplace_back(&WorkerPool::run, this, i);
  }

  ~WorkerPool() {
    {
      std::lock_guard lk(mtx_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto &t : threads_)
      if (t.joinable()) t.join();
  }

  // Run `count` variants of `base` in parallel, writing variant i into
  // out[offset + i]. `out` must already be sized; keeping it alive across
  // batches means the per-variant states keep their capacity and a batch
  // does no allocation. Each variant i runs with its own PRNG stream
  // mixSeed(batchSeed, i), making the outcome independent of scheduling.
  void runParallel(const CompactFunc &cf, const CompactState &base,
                   std::vector<RoundResult> &out, int offset, int count,
                   uint32_t batchSeed) {
    out_ = &out;
    offset_ = offset;
    nextIdx_.store(0, std::memory_order_release);
    batchSeed_.store(batchSeed, std::memory_order_release);

    {
      std::lock_guard lk(mtx_);
      cf_ = &cf;
      base_ = &base;
      batchCount_ = count;
      activeWorkers_ = static_cast<int>(threads_.size());
      ++batchGen_;
    }
    cv_.notify_all();

    // Caller helps drain the queue
    workBatch(count);

    // Rendezvous: every worker passes through workBatch exactly once per
    // generation and checks out below. Only after the last check-out is
    // results_ fully written and base_ guaranteed unused, so tearing down
    // (or starting the next batch) cannot race a straggler.
    {
      std::unique_lock lk(mtx_);
      cv_.wait(lk, [&] { return activeWorkers_ == 0; });
      base_ = nullptr;
      cf_ = nullptr;
      out_ = nullptr;
    }
  }

private:
  std::vector<std::thread> threads_;
  std::vector<RoundResult> *out_ = nullptr;
  int offset_ = 0;
  std::atomic<size_t> nextIdx_{0};
  std::atomic<uint32_t> batchSeed_{0};
  const CompactFunc *cf_ = nullptr;
  const CompactState *base_ = nullptr;
  int batchCount_ = 0;
  int activeWorkers_ = 0;  // guarded by mtx_
  uint64_t batchGen_ = 0;  // guarded by mtx_
  bool stop_ = false;
  std::mutex mtx_;
  std::condition_variable cv_;

  void workBatch(int count) {
    uint32_t batchSeed = batchSeed_.load(std::memory_order_acquire);
    while (true) {
      size_t idx = nextIdx_.fetch_add(1, std::memory_order_acq_rel);
      if ((int)idx >= count) break;
      setSeed(mixSeed(batchSeed, static_cast<uint32_t>(idx)));
      reorderRound(*cf_, *base_, (*out_)[offset_ + idx]);
    }
  }

  void run(int id) {
    (void)id; // per-variant seeding happens in workBatch
    uint64_t seenGen = 0;
    while (true) {
      int count;
      {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, [&] { return stop_ || batchGen_ != seenGen; });
        if (stop_) return;
        seenGen = batchGen_;
        count = batchCount_;
      }
      workBatch(count);
      {
        std::lock_guard lk(mtx_);
        --activeWorkers_;
      }
      cv_.notify_all();
    }
  }
};

// --- Cumulative stats --------------------------------------------------

static int64_t g_totalIterations = 0;
static double g_totalWallMs = 0.0;

void printCumulativeStats() {
  if (g_totalIterations == 0) return;
  double ips = g_totalWallMs > 0.0
                   ? g_totalIterations / (g_totalWallMs / 1000.0)
                   : 0.0;
  std::cerr << "\n=== Reorder Summary =======================" << std::endl;
  std::cerr << "  Total iterations: " << g_totalIterations << std::endl;
  std::cerr << "  Total wall time: " << std::fixed << std::setprecision(1)
            << g_totalWallMs << " ms" << std::endl;
  std::cerr << "  IPS: " << std::setprecision(0) << ips << std::endl;
}

void asmOptimize(AsmFunc &func, int maxTimeMs, int optWorkers,
                 uint32_t optSeed, int optIters, bool optAnneal) {
  const std::string &funcName =
      func.name.empty() ? "(???)" : func.name;

  asmInitDeps(func);

  const bool iterMode = optIters > 0;
  if (iterMode) {
    std::cerr << "Starting optimization of '" << funcName << "' with "
              << optIters << " iterations" << std::endl;
  } else {
    std::cerr << "Starting optimization of '" << funcName
              << "' with max. time: " << formatTimeMs(maxTimeMs) << std::endl;
  }

  // Base seed: fixed when given (reproducible builds), system entropy
  // otherwise. All variant/escape streams derive
  // from this via mixSeed, so a fixed base seed + fixed iteration count
  // + fixed worker count reproduces the exact same schedule.
  uint32_t baseSeed = optSeed;
  if (baseSeed == 0) {
    std::random_device rd;
    baseSeed = rd();
    if (baseSeed == 0) baseSeed = 0x41C64E6D;
  } else {
    std::cerr << "[" << funcName << "] Seed: " << baseSeed << std::endl;
  }
  setSeed(mixSeed(baseSeed, 0xA11C0DE));

  // Create worker pool (one thread per hardware core, minus calling thread)
  int numWorkers = optWorkers > 0
      ? optWorkers
      : std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 1);
  WorkerPool pool(numWorkers);
  std::cerr << "[" << funcName << "] Worker pool: " << numWorkers
            << " threads" << std::endl;

  // Compact representation: `best` is what gets emitted at the end, `cur`
  // the state variants are generated from (identical unless annealing).
  CompactFunc cf = compactBuild(func);
  if (!cf.plan.valid)
    std::cerr << "[" << funcName << "] NOTE: no unit plan, evaluating with a "
                 "full hot-path walk per variant" << std::endl;
  CompactState best = compactInitialState(cf);
  int costBest = compactEvalCost(cf, best);
  const int costInit = costBest;
  int sizeBest = countOpsSeq(cf, best);
  func.cyclesBefore = best.hotCycles;
  // Simulated annealing: costs are integer-scaled (one hot-path cycle = 64),
  // so T0 accepts a 1-cycle-worse move with ~30% and the final T practically
  // never.
  CompactState cur = best;
  int costCur = costBest;
  const double annealT0 = 64.0 / std::log(1.0 / 0.30);
  const double annealTend = 4.0;
  if (optAnneal)
    std::cerr << "[" << funcName << "] Annealing acceptance enabled (T "
              << (int)annealT0 << " -> " << (int)annealTend << ")" << std::endl;

  auto startTime = std::chrono::steady_clock::now();
  auto deadline = startTime + std::chrono::milliseconds(maxTimeMs);

  int i = 0;
  int metaIter = 0;
  int stepsSinceLastOpt = 0;
  int consecutiveSame = 0;
  double totalTime = 0.0;
  auto iterStart = startTime;

  // Persistent across meta-iterations: the variant states inside keep
  // their capacity, so steady-state batches allocate nothing.
  std::vector<RoundResult> results;
  RoundResult escapeScratch;

  while (iterMode ? (metaIter < optIters) : (totalTime < maxTimeMs)) {
    auto now = std::chrono::steady_clock::now();

    // Progress logging
    if (metaIter < 5 || (metaIter % PROGRESS_LOG_INTERVAL) == 0) {
      auto dur = std::chrono::duration<double, std::milli>(now - iterStart)
                     .count();
      totalTime += dur;
      double wallSec =
          std::chrono::duration<double>(now - startTime).count();
      double ips = wallSec > 0.0 ? i / wallSec : 0.0;
      double left = maxTimeMs - totalTime;
      std::cerr << "[" << funcName << "] Step: " << i
                << ", Left: " << std::fixed << std::setprecision(1) << left
                << "ms | Cost: " << costBest
                << " | ips: " << std::setprecision(0) << ips;
      std::cerr << std::endl;
      iterStart = now;
    }

    // Check timeout (wall-clock mode only; iteration mode runs exactly
    // optIters meta-iterations regardless of time)
    if (!iterMode && now > deadline) {
      std::cerr << "[" << funcName << "] Timeout after " << i
                << " iterations." << std::endl;
      break;
    }

    // Every stream in this meta-iteration derives from (baseSeed, metaIter):
    // the parallel variants via mixSeed(batchSeed, variantIdx) inside the
    // pool, the sequential escape path via its own sub-stream below.
    uint32_t batchSeed = mixSeed(baseSeed, static_cast<uint32_t>(metaIter));

    int effectivePool = POOL_SIZE * numWorkers;

    if (stepsSinceLastOpt > MAX_STEPS_NO_CHANGE) {
      ++consecutiveSame;
      int stepsBack = consecutiveSame * SEARCH_BACK_STEPS_FACTOR;
      int stepsFwd = consecutiveSame * SEARCH_FWD_STEPS_FACTOR;
      std::cerr << "[" << funcName << "] " << stepsSinceLastOpt
                << " steps since last improvement, generate new versions ("
                << stepsBack << " steps backward)" << std::endl;

      // Escape local minimum: generate worse variants (sequential, each
      // uses many reorderRound calls internally), then finalize in parallel.
      int remaining = std::max(0, effectivePool - SEARCH_VARIANT_SEARCH);
      results.resize(SEARCH_VARIANT_SEARCH + remaining);
      setSeed(mixSeed(batchSeed, 0xE5CA9Eu));
      for (int s = 0; s < SEARCH_VARIANT_SEARCH; ++s) {
        auto [worseCopy, maxCost] = generateWorseFunction(cf, best, stepsBack);
        // walk part of the way back down from the worse state
        for (int t = 0; t < stepsFwd; ++t) {
          reorderRound(cf, worseCopy, escapeScratch);
          if (escapeScratch.cost < maxCost) {
            worseCopy = escapeScratch.st;
            maxCost = escapeScratch.cost;
          }
        }
        reorderRound(cf, worseCopy, results[s]);
      }
      // Remaining pool slots: if any left, run in parallel.
      if (remaining > 0) {
        pool.runParallel(cf, optAnneal ? cur : best, results,
                         SEARCH_VARIANT_SEARCH, remaining, batchSeed);
      }
      stepsSinceLastOpt = 0;
    } else {
      results.resize(effectivePool);
      pool.runParallel(cf, optAnneal ? cur : best, results, 0, effectivePool,
                       batchSeed);
    }
    if (optAnneal) {
      // pick the best variant of this batch, accept it into `cur` if it is
      // not worse, or with probability exp(-delta/T) otherwise; track best
      int bi = -1;
      for (int s = 0; s < (int)results.size(); ++s) {
        if (results[s].cost == 0) continue;
        if (bi < 0 || results[s].cost < results[bi].cost) bi = s;
      }
      if (bi >= 0) {
        int cost = results[bi].cost;
        CompactState &stv = results[bi].st;
        double frac = iterMode ? (double)metaIter / std::max(1, optIters)
                               : std::min(1.0, totalTime / std::max(1, maxTimeMs));
        double T = annealT0 * std::pow(annealTend / annealT0, frac);
        std::mt19937 arng(mixSeed(batchSeed, 0xA11EA7u));
        double u = std::uniform_real_distribution<double>(0.0, 1.0)(arng);
        bool accept = cost <= costCur || u < std::exp(-(double)(cost - costCur) / T);
        if (accept) { cur = stv; costCur = cost; }
        int opCount = countOpsSeq(cf, stv);
        bool isBetter = cost < costBest || (cost == costBest && opCount < sizeBest);
        if (isBetter) {
          costBest = cost; sizeBest = opCount; best = stv;
          std::cerr << "[" << funcName << "] \033[32m**** New Best for '"
                    << funcName << "': " << costInit << " -> " << cost
                    << " (" << opCount << " ops) ****\033[0m" << std::endl;
          stepsSinceLastOpt = 0;
          consecutiveSame = 0;
        }
      }
    } else
    for (int s = 0; s < (int)results.size(); ++s) {
      int cost = results[s].cost;
      const CompactState &stv = results[s].st;
      // Safety: a cost of 0 means the variant is broken (no instructions or
      // dependency corruption). Reject it to prevent poisoning the result.
      if (cost == 0) continue;
      int opCount = countOpsSeq(cf, stv);
      // Cycles first; on a tie fewer instructions win (filled delay slots
      // drop a NOP, saving IMEM at identical cycle cost).
      bool isBetter = cost < costBest ||
                      (cost == costBest && opCount < sizeBest);
      bool isSame = cost == costBest && opCount == sizeBest;
      bool canUseTheSame = s < ((int)results.size() / 4);

      if (isBetter || (canUseTheSame && isSame)) {
        costBest = cost;
        sizeBest = opCount;
        best = stv;

        if (isBetter) {
          std::cerr << "[" << funcName << "] \033[32m**** New Best for '"
                    << funcName << "': " << costInit << " -> " << cost
                    << " (" << opCount << " ops) ****\033[0m" << std::endl;
          stepsSinceLastOpt = 0;
          consecutiveSame = 0;
        }
      }
    }

    i += effectivePool;
    ++metaIter;
    ++stepsSinceLastOpt;
  }

  {
    double funcElapsedMs = std::chrono::duration<double, std::milli>(
                               std::chrono::steady_clock::now() - startTime)
                               .count();
    g_totalIterations += i;
    g_totalWallMs += funcElapsedMs;
  }

  // Materialize the best order back into the function.
  compactApply(cf, best, func);

  // Human-readable numbers: hot-path cycles (the objective itself is a
  // weighted, integer-scaled cost).
  asmInitDeps(func);
  evalFunctionCost(func);
  func.cyclesAfter = func.hotCycles;
  func.costBefore = costInit;
  func.costAfter = costBest;
}

} // namespace rspl
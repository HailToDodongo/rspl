#include "asm_optimizer.h"
#include "asm.h"
#include "asm_scan_deps.h"
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

// --- PRNG (matches JS LCG, thread-local for worker parallelism) ----------

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
  for (size_t i = 0; i < func.asm_.size(); ++i) {
    auto &inst = func.asm_[i];
    if (inst.type != AsmType::OP) continue;
    if (inst.opFlags &
        (OpFlag::OP_FLAG_IS_IMMOVABLE | OpFlag::OP_FLAG_IS_NOP |
         OpFlag::OP_FLAG_IS_BRANCH))
      continue;

    auto reorderRange = asmGetReorderIndices(func.asm_, static_cast<int>(i));

    int delaySlotIdx = -1;
    for (int idx : reorderRange) {
      if (idx <= static_cast<int>(i)) continue;
      if (func.asm_[idx].opFlags & OpFlag::OP_FLAG_IS_NOP) {
        delaySlotIdx = idx;
        break;
      }
    }

    if (delaySlotIdx >= 0) {
      func.asm_[delaySlotIdx] = std::move(inst);
      func.asm_.erase(func.asm_.begin() + static_cast<long>(i));
      --i;
    }
  }
}

// ==========================================================================
// Reorder optimization (stochastic annealing, matches JS algorithm)
// ==========================================================================

// --- Constants (matching JS) ----------------------------------------------

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

// IMEM usage of a variant: every OP (incl. NOPs) is one instruction word.
static int countOps(const std::vector<AsmInst> &asmList) {
  int count = 0;
  for (const auto &inst : asmList) {
    if (inst.type == AsmType::OP) ++count;
  }
  return count;
}

static AsmFunc cloneFunction(const AsmFunc &func) {
  // Only asm_ is needed by reorderRound / evalFunctionCost / asmInitDeps.
  // Skip copying name, type, argSize, annotations, etc.
  AsmFunc copy;
  copy.asm_ = func.asm_;
  return copy;
}

// Forward-declared
static std::pair<AsmFunc, int> generateWorseFunction(const AsmFunc &base,
                                                     int steps);

// --- relocateElement (matches JS relocateElement) --------------------------

static void relocateElement(std::vector<AsmInst> &arr, int from, int to) {
  if (from == to) return;
  if (arr[to].opFlags & OpFlag::OP_FLAG_IS_BRANCH) return;
  bool targetIsNOP = arr[to].opFlags & OpFlag::OP_FLAG_IS_NOP;
  bool sourceInDelaySlot =
      (from >= 1) && (arr[from - 1].opFlags & OpFlag::OP_FLAG_IS_BRANCH);

  if (sourceInDelaySlot) {
    if (targetIsNOP) {
      // Replace NOP with delay-slot instruction (keep delay slot filled)
      arr[to] = arr[from];
    } else {
      AsmInst inst = std::move(arr[from]);
      arr[from] = asmNOP();
      asmInitDep(arr[from]);
      arr.insert(arr.begin() + to, std::move(inst));
    }
  } else {
    if (targetIsNOP) {
      arr[to] = std::move(arr[from]);
      arr.erase(arr.begin() + from);
    } else {
      AsmInst inst = std::move(arr[from]);
      arr.erase(arr.begin() + from);
      if (to > from) to--;
      arr.insert(arr.begin() + to, std::move(inst));
    }
  }
}

// --- optimizeStep (matches JS optimizeStep) --------------------------------

static bool rebaseHopEnabled() {
  static const bool enabled = [] {
    const char *e = std::getenv("RSPL_REBASE_HOP");
    return !(e && e[0] == '0');
  }();
  return enabled;
}

static int optimizeStep(AsmFunc &func) {
  auto sz = static_cast<int>(func.asm_.size());
  if (sz < 2) return 0;

  // Occasionally try an offset-rebase hop; most picks are not rebasable
  // mem-ops and fall through to the normal move below at trivial cost.
  if (rebaseHopEnabled() && rand01() < REBASE_HOP_RATE) {
    int hopIdx = randIndex(sz);
    bool fwd = rand01() < 0.5;
    if (asmTryRebaseCross(func.asm_, hopIdx, fwd) ||
        asmTryRebaseCross(func.asm_, hopIdx, !fwd)) {
      return 1;
    }
  }

  int i = 0;
  std::vector<int> reorderIndices;
  for (int r = 0; r < 50; ++r) {
    i = randIndex(sz);
    reorderIndices = asmGetReorderIndices(func.asm_, i);
    if ((int)reorderIndices.size() > 1) break;
  }
  if ((int)reorderIndices.size() <= 1) return 0;

  int targetIdx = i;
  bool foundIndex = false;

  // Prefer pairing opposite-type (vector<->scalar) unpaired instructions
  if (rand01() < PREFER_PAIR_RATE) {
    for (int j : reorderIndices) {
      if ((func.asm_[j].opFlags & OpFlag::OP_FLAG_IS_VECTOR) !=
          (func.asm_[i].opFlags & OpFlag::OP_FLAG_IS_VECTOR)) {
        if (!func.asm_[j].debug.paired) {
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
      int stalls = func.asm_[j].debug.stall;
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

  func.asm_[i].debug.reorderCount++;
  relocateElement(func.asm_, i, targetIdx);
  return 1;
}

// --- reorderRound (matches JS reorderRound) --------------------------------

struct RoundResult {
  int cost;
  std::vector<AsmInst> asm_;
};

// --- Phase-level timing (printed every N iterations) --------------------

struct PhaseTiming {
  double cloneMs = 0;
  double reorderMs = 0;
  double depsMs = 0;
  double evalMs = 0;
  double dispatchMs = 0;
  double resultsMs = 0;
  int samples = 0;
  void reset() { *this = {}; }
};
PhaseTiming g_phaseTiming;

// Mutable variant: takes ownership, avoids double-clone from WorkerPool.
static RoundResult reorderRoundImpl(AsmFunc func) {
  int opCount = randIndex(REORDER_MAX_OPS - REORDER_MIN_OPS) + REORDER_MIN_OPS;
  for (int o = 0; o < opCount; ++o)
    optimizeStep(func);
  // optimizeStep already keeps dep data current via asmInitDep calls inside
  // relocateElement. Bulk rescan is redundant — deps are position-independent.
  int cost = evalFunctionCost(func);
  return {cost, std::move(func.asm_)};
}

static RoundResult reorderRound(const AsmFunc &baseFunc) {
  return reorderRoundImpl(cloneFunction(baseFunc));
}

// --- generateWorseFunction (matches JS generateWorseFunction) ---------------

static std::pair<AsmFunc, int> generateWorseFunction(const AsmFunc &base,
                                                     int steps) {
  int maxCost = 0;
  AsmFunc newWorst = cloneFunction(base);
  for (int i = 0; i < steps; ++i) {
    AsmFunc f = cloneFunction(base);
    reorderRound(f);
    reorderRound(f);
    asmInitDeps(f);
    int cost = evalFunctionCost(f);
    if (cost > maxCost) {
      newWorst = std::move(f);
      maxCost = cost;
    }
  }
  return {std::move(newWorst), maxCost};
}

// --- Helper: format time string from ms -----------------------------------

static std::string formatTimeMs(int ms) {
  int h = ms / 3600000;
  int m = (ms % 3600000) / 60000;
  int s = (ms % 60000) / 1000;
  std::ostringstream ss;
  ss << std::setfill('0') << std::setw(2) << h << ":";
  ss << std::setfill('0') << std::setw(2) << m << ":";
  ss << std::setfill('0') << std::setw(2) << s;
  return ss.str();
}

// --- Parallel variant execution -------------------------------------------

// Each worker runs a full variant (clone → reorderRound) independently.
// The caller dispatches a batch of N variants; all threads (including caller)
// pull from a shared index. Results go into a freshly-allocated vector so
// there's no reuse of moved-from state between calls.

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

  // Run `count` variants of `base` in parallel. Returns results.
  // Each variant i runs with its own PRNG stream mixSeed(batchSeed, i),
  // making the batch outcome independent of thread scheduling.
  std::vector<RoundResult> runParallel(const AsmFunc &base, int count,
                                       uint32_t batchSeed) {
    results_.clear();
    results_.resize(count);
    nextIdx_.store(0, std::memory_order_release);
    batchSeed_.store(batchSeed, std::memory_order_release);

    {
      std::lock_guard lk(mtx_);
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
    }

    std::vector<RoundResult> out;
    results_.swap(out);
    return out;
  }

private:
  std::vector<std::thread> threads_;
  std::vector<RoundResult> results_;
  std::atomic<size_t> nextIdx_{0};
  std::atomic<uint32_t> batchSeed_{0};
  const AsmFunc *base_ = nullptr;
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
      AsmFunc variant = cloneFunction(*base_);
      results_[idx] = reorderRoundImpl(std::move(variant));
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

// --- asmOptimize (matches JS asmOptimize) ----------------------------------

// --- Cumulative perf counters -------------------------------------------

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
  int costBest = evalFunctionCost(func);
  int sizeBest = countOps(func.asm_);
  func.cyclesBefore = func.hotCycles;
  int costInit = costBest;

  const bool iterMode = optIters > 0;
  if (iterMode) {
    std::cerr << "Starting optimization of '" << funcName << "' with "
              << optIters << " iterations" << std::endl;
  } else {
    std::cerr << "Starting optimization of '" << funcName
              << "' with max. time: " << formatTimeMs(maxTimeMs) << std::endl;
  }

  // Base seed: fixed when given (reproducible builds), system entropy
  // otherwise (JS uses Math.random). All variant/escape streams derive
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

  AsmFunc lastRandPick = cloneFunction(func);

  // Simulated annealing: `cur` is the state variants are generated from;
  // `func` always holds the best state seen (what gets emitted).
  // Costs are integer-scaled (one hot-path cycle = 64), so T0 accepts a
  // 1-cycle-worse move with ~30% and the final T practically never.
  AsmFunc cur = cloneFunction(func);
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

      // Phase breakdown (every PROGRESS_LOG_INTERVAL meta-iterations)
      if (metaIter > 0 && (metaIter % PROGRESS_LOG_INTERVAL) == 0 &&
          g_phaseTiming.samples > 0) {
        double total = g_phaseTiming.cloneMs + g_phaseTiming.reorderMs +
                       g_phaseTiming.depsMs + g_phaseTiming.evalMs +
                       g_phaseTiming.dispatchMs + g_phaseTiming.resultsMs;
        auto pct = [&](double v) { return (int)(v / total * 100); };
        std::cerr << "\n  [profile] clone:" << pct(g_phaseTiming.cloneMs)
                  << "% reorder:" << pct(g_phaseTiming.reorderMs)
                  << "% deps:" << pct(g_phaseTiming.depsMs)
                  << "% eval:" << pct(g_phaseTiming.evalMs)
                  << "% dispatch:" << pct(g_phaseTiming.dispatchMs)
                  << "% results:" << pct(g_phaseTiming.resultsMs)
                  << "  (samples:" << g_phaseTiming.samples << ")";
      }
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

    AsmFunc funcCopy = cloneFunction(func);
    std::vector<RoundResult> results;
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
      setSeed(mixSeed(batchSeed, 0xE5CA9Eu));
      for (int s = 0; s < SEARCH_VARIANT_SEARCH; ++s) {
        auto [worseCopy, maxCost] =
            generateWorseFunction(funcCopy, stepsBack);
        for (int t = 0; t < stepsFwd; ++t) {
          AsmFunc worseCopyTry = cloneFunction(worseCopy);
          reorderRound(worseCopyTry);
          asmInitDeps(worseCopyTry);
          int cost = evalFunctionCost(worseCopyTry);
          if (cost < maxCost) {
            worseCopy.asm_ = std::move(worseCopyTry.asm_);
            maxCost = cost;
          }
        }
        // Finalize escape variant via reorderRound
        AsmFunc variant = cloneFunction(worseCopy);
        results.push_back(reorderRound(variant));
      }
      // Remaining pool slots: if any left, run in parallel.
      int remaining = effectivePool - SEARCH_VARIANT_SEARCH;
      if (remaining > 0) {
        auto tD0 = std::chrono::steady_clock::now();
        auto extraResults = pool.runParallel(optAnneal ? cur : func, remaining, batchSeed);
        g_phaseTiming.dispatchMs +=
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - tD0).count();
        results.insert(results.end(),
                       std::make_move_iterator(extraResults.begin()),
                       std::make_move_iterator(extraResults.end()));
      }
      stepsSinceLastOpt = 0;
    } else {
      auto tD0 = std::chrono::steady_clock::now();
      results = pool.runParallel(optAnneal ? cur : func, effectivePool, batchSeed);
      g_phaseTiming.dispatchMs +=
          std::chrono::duration<double, std::milli>(
              std::chrono::steady_clock::now() - tD0).count();
    }

    auto tR0 = std::chrono::steady_clock::now();
    if (optAnneal) {
      // pick the best variant of this batch, accept it into `cur` if it is
      // not worse, or with probability exp(-delta/T) otherwise; track best
      int bi = -1;
      for (int s = 0; s < (int)results.size(); ++s) {
        const auto &[cost, asm_] = results[s];
        if (cost == 0) continue;
        if (bi < 0 || cost < results[bi].cost) bi = s;
      }
      if (bi >= 0) {
        const auto &[cost, asm_] = results[bi];
        double frac = iterMode ? (double)metaIter / std::max(1, optIters)
                               : std::min(1.0, totalTime / std::max(1, maxTimeMs));
        double T = annealT0 * std::pow(annealTend / annealT0, frac);
        std::mt19937 arng(mixSeed(batchSeed, 0xA11EA7u));
        double u = std::uniform_real_distribution<double>(0.0, 1.0)(arng);
        bool accept = cost <= costCur || u < std::exp(-(double)(cost - costCur) / T);
        if (accept) { cur.asm_ = asm_; costCur = cost; }
        int opCount = countOps(asm_);
        bool isBetter = cost < costBest || (cost == costBest && opCount < sizeBest);
        if (isBetter) {
          costBest = cost; sizeBest = opCount; func.asm_ = asm_;
          std::cerr << "[" << funcName << "] \033[32m**** New Best for '"
                    << funcName << "': " << costInit << " -> " << cost
                    << " (" << opCount << " ops) ****\033[0m" << std::endl;
          stepsSinceLastOpt = 0;
          consecutiveSame = 0;
        }
      }
      results.clear(); // handled
    }
    for (int s = 0; s < (int)results.size(); ++s) {
      const auto &[cost, asm_] = results[s];
      // Safety: a cost of 0 means the variant is broken (no instructions or
      // dependency corruption). Reject it to prevent poisoning func.asm_.
      if (cost == 0) continue;
      int opCount = countOps(asm_);
      // Cycles first; on a tie fewer instructions win (filled delay slots
      // drop a NOP, saving IMEM at identical cycle cost).
      bool isBetter = cost < costBest ||
                      (cost == costBest && opCount < sizeBest);
      bool isSame = cost == costBest && opCount == sizeBest;
      bool canUseTheSame = s < ((int)results.size() / 4);

      if (isBetter || (canUseTheSame && isSame)) {
        costBest = cost;
        sizeBest = opCount;
        func.asm_ = asm_;

        if (isBetter) {
          std::cerr << "[" << funcName << "] \033[32m**** New Best for '"
                    << funcName << "': " << costInit << " -> " << cost
                    << " (" << opCount << " ops) ****\033[0m" << std::endl;
          stepsSinceLastOpt = 0;
          consecutiveSame = 0;
        }
      }
    }

    g_phaseTiming.resultsMs +=
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - tR0).count();

    if (i % 3 == 0) lastRandPick = funcCopy;
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

  // Human-readable numbers: hot-path cycles (the objective itself is a
  // weighted, integer-scaled cost).
  asmInitDeps(func);
  evalFunctionCost(func);
  func.cyclesAfter = func.hotCycles;
}

} // namespace rspl
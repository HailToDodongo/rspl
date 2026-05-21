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
constexpr int MAX_STEPS_NO_CHANGE = 5000;
constexpr int SEARCH_VARIANT_SEARCH = 10;
constexpr int SEARCH_BACK_STEPS_FACTOR = 10;
constexpr int SEARCH_FWD_STEPS_FACTOR = 5;
constexpr int REORDER_MIN_OPS = 3;
constexpr int REORDER_MAX_OPS = 15;

// --- Helper functions -----------------------------------------------------

static AsmFunc cloneFunction(const AsmFunc &func) {
  AsmFunc copy = func;
  copy.asm_ = func.asm_; // vector copy
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

static int optimizeStep(AsmFunc &func) {
  auto sz = static_cast<int>(func.asm_.size());
  if (sz < 2) return 0;

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

static RoundResult reorderRound(const AsmFunc &baseFunc) {
  AsmFunc func = cloneFunction(baseFunc);
  int opCount = randIndex(REORDER_MAX_OPS - REORDER_MIN_OPS) + REORDER_MIN_OPS;
  for (int o = 0; o < opCount; ++o) {
    optimizeStep(func);
  }
  asmInitDeps(func);
  int cost = evalFunctionCost(func);
  return {cost, std::move(func.asm_)};
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

// Spawn worker threads once and reuse them across iterations.
// Each batch of variants is dispatched via runParallel: all threads
// (including the caller) pull tasks from a shared index, run reorderRound
// outside the lock, and push results back under the lock.

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

  // Dispatch a batch of tasks. Blocks until all complete. Returns results.
  std::vector<RoundResult> runParallel(std::vector<AsmFunc> tasks) {
    size_t total = tasks.size();
    tasks_ = std::move(tasks);
    results_.resize(total);
    nextIdx_.store(0, std::memory_order_release);
    doneCount_.store(0, std::memory_order_release);

    // Wake workers
    {
      std::lock_guard lk(mtx_);
      batchTotal_ = total;
      batchActive_ = true;
    }
    cv_.notify_all();

    // Caller participates
    workBatch();

    // Spin-wait for stragglers (workers finish quickly since caller did
    // most of the heavy work; this avoids a CV roundtrip)
    while (doneCount_.load(std::memory_order_acquire) < total) {
      // help out if any tasks remain
      workBatch();
    }

    // Signal batch done
    {
      std::lock_guard lk(mtx_);
      batchActive_ = false;
      batchTotal_ = 0;
    }

    std::vector<RoundResult> out;
    out.reserve(total);
    for (auto &r : results_)
      out.push_back(std::move(r));
    results_.clear();
    return out;
  }

private:
  std::vector<std::thread> threads_;
  std::vector<AsmFunc> tasks_;
  std::vector<RoundResult> results_;
  std::atomic<size_t> nextIdx_{0};
  std::atomic<size_t> doneCount_{0};
  size_t batchTotal_ = 0;
  bool batchActive_ = false;
  bool stop_ = false;
  std::mutex mtx_;
  std::condition_variable cv_;

  void workBatch() {
    size_t total = batchTotal_;
    while (true) {
      size_t idx = nextIdx_.fetch_add(1, std::memory_order_acq_rel);
      if (idx >= total) break;
      AsmFunc task = std::move(tasks_[idx]);
      RoundResult r = reorderRound(task);
      results_[idx] = std::move(r);
      doneCount_.fetch_add(1, std::memory_order_release);
    }
  }

  void run(int id) {
    // Unique seed per worker
    std::random_device rd;
    setSeed(rd() ^ (static_cast<uint32_t>(id) * 0x9E3779B9));

    while (true) {
      {
        std::unique_lock lk(mtx_);
        cv_.wait(lk, [&] { return stop_ || batchActive_; });
        if (stop_) return;
      }
      workBatch();
    }
  }
};

// --- asmOptimize (matches JS asmOptimize) ----------------------------------

// --- Cumulative perf counters -------------------------------------------

static int64_t g_totalIterations = 0;
static double g_totalElapsedMs = 0.0;

void printCumulativeStats() {
  if (g_totalIterations == 0) return;
  double avgIps = g_totalElapsedMs > 0.0
                      ? g_totalIterations / (g_totalElapsedMs / 1000.0)
                      : 0.0;
  std::cerr << "\n=== Reorder Summary =======================" << std::endl;
  std::cerr << "  Total iterations: " << g_totalIterations << std::endl;
  std::cerr << "  Total time: " << std::fixed << std::setprecision(1)
            << g_totalElapsedMs << " ms" << std::endl;
  std::cerr << "  Average IPS: " << std::setprecision(0) << avgIps
            << std::endl;
}

void asmOptimize(AsmFunc &func, int maxTimeMs) {
  const std::string &funcName =
      func.name.empty() ? "(???)" : func.name;

  asmInitDeps(func);
  int costBest = evalFunctionCost(func);
  func.cyclesBefore = costBest;
  int costInit = costBest;

  std::cerr << "Starting optimization of '" << funcName
            << "' with max. time: " << formatTimeMs(maxTimeMs) << std::endl;

  // Initialize random seed from system entropy (JS uses Math.random)
  std::random_device rd;
  setSeed(rd());

  // Create worker pool (one thread per hardware core, minus calling thread)
  unsigned hwThreads = std::thread::hardware_concurrency();
  int numWorkers = std::max(1, static_cast<int>(hwThreads) - 1);
  WorkerPool pool(numWorkers);
  std::cerr << "[" << funcName << "] Worker pool: " << numWorkers
            << " threads" << std::endl;

  AsmFunc lastRandPick = cloneFunction(func);

  auto startTime = std::chrono::steady_clock::now();
  auto deadline = startTime + std::chrono::milliseconds(maxTimeMs);

  int i = 0;
  int stepsSinceLastOpt = 0;
  int consecutiveSame = 0;
  double totalTime = 0.0;
  auto iterStart = startTime;

  while (totalTime < maxTimeMs) {
    auto now = std::chrono::steady_clock::now();

    // Progress logging (first 10 individually, then every 1000)
    if (i < 10 || (i % 1000) == 0) {
      auto dur = std::chrono::duration<double, std::milli>(now - iterStart)
                     .count();
      totalTime += dur;
      double elapsedSec = totalTime / 1000.0;
      double ips = elapsedSec > 0.0 ? i / elapsedSec : 0.0;
      double left = maxTimeMs - totalTime;
      std::cerr << "[" << funcName << "] Step: " << i
                << ", Left: " << std::fixed << std::setprecision(1) << left
                << "ms | Cost: " << costBest
                << " | ips: " << std::setprecision(0) << ips << std::endl;
      iterStart = now;
    }

    // Check timeout
    if (now > deadline) {
      double funcElapsedMs =
          std::chrono::duration<double, std::milli>(now - startTime).count();
      double funcIps =
          funcElapsedMs > 0.0 ? i / (funcElapsedMs / 1000.0) : 0.0;
      g_totalIterations += i;
      g_totalElapsedMs += funcElapsedMs;
      double cumIps = g_totalElapsedMs > 0.0
                          ? g_totalIterations / (g_totalElapsedMs / 1000.0)
                          : 0.0;
      std::cerr << "[" << funcName << "] Timeout after " << i
                << " iterations (" << std::fixed << std::setprecision(0)
                << funcIps << " ips func, "
                << cumIps << " ips cum)." << std::endl;
      break;
    }

    AsmFunc funcCopy = cloneFunction(func);
    std::vector<RoundResult> results;

    if (stepsSinceLastOpt > MAX_STEPS_NO_CHANGE) {
      ++consecutiveSame;
      int stepsBack = consecutiveSame * SEARCH_BACK_STEPS_FACTOR;
      int stepsFwd = consecutiveSame * SEARCH_FWD_STEPS_FACTOR;
      std::cerr << "[" << funcName << "] " << stepsSinceLastOpt
                << " steps since last improvement, generate new versions ("
                << stepsBack << " steps backward)" << std::endl;

      // Escape local minimum: generate worse variants (sequential, each
      // uses many reorderRound calls internally), then finalize in parallel.
      std::vector<AsmFunc> escapeTasks;
      escapeTasks.reserve(POOL_SIZE);
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
        escapeTasks.push_back(cloneFunction(worseCopy));
      }
      for (int s = SEARCH_VARIANT_SEARCH; s < POOL_SIZE; ++s) {
        escapeTasks.push_back(
            cloneFunction(rand01() < 0.1 ? lastRandPick : funcCopy));
      }
      results = pool.runParallel(std::move(escapeTasks));
      stepsSinceLastOpt = 0;
    } else {
      std::vector<AsmFunc> tasks;
      tasks.reserve(POOL_SIZE);
      for (int s = 0; s < POOL_SIZE; ++s) {
        tasks.push_back(
            cloneFunction(rand01() < 0.1 ? lastRandPick : funcCopy));
      }
      results = pool.runParallel(std::move(tasks));
    }

    for (int s = 0; s < (int)results.size(); ++s) {
      const auto &[cost, asm_] = results[s];
      bool isBetter = cost < costBest;
      bool isSame = cost == costBest;
      bool canUseTheSame = s < ((int)results.size() / 4);

      if (isBetter || (canUseTheSame && isSame)) {
        costBest = cost;
        func.asm_ = asm_;
        func.cyclesAfter = cost;

        if (isBetter) {
          std::cerr << "[" << funcName << "] \033[32m**** New Best for '"
                    << funcName << "': " << costInit << " -> " << cost
                    << " ****\033[0m" << std::endl;
          stepsSinceLastOpt = 0;
          consecutiveSame = 0;
        }
      }
    }

    if (i % 3 == 0) lastRandPick = funcCopy;
    ++i;
    ++stepsSinceLastOpt;
  }

  func.cyclesBefore = costInit;
  func.cyclesAfter = costBest;
}

} // namespace rspl
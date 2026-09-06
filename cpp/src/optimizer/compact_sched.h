#pragma once
// Compact scheduling representation for the reorder search.
//
// The annealer's inner loop used to clone a whole AsmFunc (vectors of
// AsmInst with strings) per variant, move elements, re-derive dependency
// data and evaluate. Everything that changes between variants is the
// ORDER; everything else about an instruction is fixed. So:
//   - CompactFunc: one row per item (op / label / the shared NOP) with the
//     integer data the move-range, relocate, rebase-hop and eval logic need
//   - CompactState: the item order (plus per-op rebased offsets and the
//     per-op eval feedback the move heuristics read)
// This is the one implementation of the move rules and the cost model; the
// AsmInst-based entry points (asmGetReorderIndices, asmTryRebaseCross,
// evalFunctionCost, ...) are thin wrappers over it.
#include "../asm.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace rspl {

struct CompactOp {
  bool isOp = false;      // OP entry (labels and other entries: false)
  bool isNop = false;
  uint32_t flags = 0;     // OpFlag bits
  bool isJ = false, isJal = false, isJr = false, isUncond = false;
  int stallLatency = 0;
  uint32_t barrier = 0;
  std::array<uint64_t, 5> src{}, tgt{}, arg{};
  uint32_t stSrc0 = 0, stSrc1 = 0, stTgt0 = 0, stTgt1 = 0;
  SmallVec<int, 24> tgtIdx;   // REG_INDEX targets (WAW / RAW tracking)
  SmallVec<int, 24> stSrc, stTgt; // stall-index sources / targets
  int targetLabel = -1;   // branch: item id of the target label, -1 = outside
  // rebase hop data
  RebaseKind rebaseKind = RebaseKind::None;
  int16_t rebaseBase = -1;
  int32_t rebaseValue = 0;   // initial value; per-variant copy in the state
  int rebaseScale = 0;       // vector mem op element scale, 0 = scalar
};

// Order-invariant hot-path structure. Items only ever move inside a *unit*
// (a body run delimited by labels/branches, a branch, or a branch's delay
// slot; labels and branches themselves are immovable), so the walk over
// units — which are hot and in what order, their loop weight, which are
// alternative arms or cold code — is the same for every variant. It is
// derived once from the initial order; eval only re-derives the units'
// current item ranges and simulates.
struct CompactPlan {
  std::vector<int> hotUnits;    // unit ids in visit order
  std::vector<int> hotWeights;  // per hotUnits entry
  struct Run { std::vector<int> units; int weight = 0; };
  std::vector<Run> runs;        // maximal unvisited unit runs, in order
  int unitCount = 0;
  bool valid = false;           // false: eval falls back to a full walk
};

// $acc chains (docs/plan-mac-group-reorder.md). Every op touching the
// accumulator belongs to exactly one chain: an op that writes $acc without
// reading it starts a chain, each following reader extends it. Membership
// is derived from the initial order and never changes during the search
// (members cannot cross each other or another chain's members, and no VU op
// can enter a chain), so the table is keyed by item id.
struct CompactChain {
  std::vector<int> ids;   // member ids in program order
  // no label / branch between the first and last member (members never
  // cross branches, so this is fixed too); only movable chains get chain
  // targets from the range scan
  bool movable = true;
};

struct CompactFunc {
  std::vector<CompactOp> ops;   // item id -> data (last entry: the NOP)
  int nopId = -1;
  std::vector<AsmInst> orig;    // item id -> original instruction (apply)
  std::vector<uint8_t> likelyHot, likelyCold; // per item id: taken-branch bubble
  CompactPlan plan;
  std::vector<CompactChain> chains;
  std::vector<int> chainOf;     // per item id: chain index, -1 = none
};

struct CompactState {
  std::vector<int> seq;            // item ids in program order
  std::vector<int32_t> rebaseValue; // per item id (mem ops)
  std::vector<int16_t> stall;      // per item id: eval feedback for heuristics
  std::vector<uint8_t> paired;     // per item id
  std::vector<int> cycle;          // per position: issue cycle of the last eval
  int hotCycles = 0;
};

CompactFunc compactBuild(const AsmFunc &func);
CompactState compactInitialState(const CompactFunc &cf);

// Options / results of the range scan beyond the plain single-item band.
struct CompactRange {
  // in: when the scan stops on an $acc dependency with a member of a
  // *foreign* chain, keep going with $acc masked and emit the positions
  // where the whole chain could land (never strictly inside another chain,
  // never in a delay slot unless the chain has a single member). Targets
  // outside [plainLo, plainHi] must go through compactRelocateChain.
  bool chainMoves = false;
  // in: treat $acc as free from the start (follower placement inside
  // compactRelocateChain); no chain-landing filter
  bool maskAcc = false;
  // out: the band of ordinary single-item targets, plainLo <= pos <= plainHi
  int plainLo = 0, plainHi = 0;
};

/// Positions the item at `pos` may legally occupy (port of asmGetReorderIndices)
std::vector<int> compactReorderIndices(const CompactFunc &cf, const CompactState &st, int pos);
/// Same, written into a caller-owned buffer (cleared first; no allocation
/// once its capacity has grown) — the annealer's hot loop uses this form.
/// A target t means "insert before the item currently at t" (or replace it
/// when it is a NOP); positions inside the plain band come first.
void compactReorderIndices(const CompactFunc &cf, const CompactState &st, int pos,
                           std::vector<int> &out, CompactRange *range = nullptr);
/// Move item from `from` to `to` (port of relocateElement, incl. NOP handling)
void compactRelocate(const CompactFunc &cf, CompactState &st, int from, int to);
/// Move the whole $acc chain of the item at `leaderPos` so that this item
/// lands at `target` (a chain target from compactReorderIndices) and the
/// other members follow it, one by one, each placed directly next to the
/// previously placed one. The leader must be the chain's last member in the
/// current order for a forward move, its first for a backward move.
/// Returns false and leaves the state untouched when a member cannot reach
/// its slot.
bool compactRelocateChain(const CompactFunc &cf, CompactState &st, int leaderPos, int target);
/// Offset-rebase hop (port of asmTryRebaseCross)
bool compactTryRebaseCross(const CompactFunc &cf, CompactState &st, int pos, bool forward);
/// Weighted path-aware cost (port of evalFunctionCost); sets st.hotCycles
/// and the per-op stall/paired feedback
int compactEvalCost(const CompactFunc &cf, CompactState &st);
/// Plain linear walk over all items (debug annotation numbering); fills
/// st.cycle and returns the total cycle count
int compactEvalCostLinear(const CompactFunc &cf, CompactState &st);
/// Materialize the state back into func.asm_ (rebased offsets rewritten,
/// dependency data re-initialized)
void compactApply(const CompactFunc &cf, const CompactState &st, AsmFunc &func);

/// Safety net for the reorder search: checks that `seq` is a legal
/// reordering of the initial order. Every op keeps its segment (labels and
/// branch/delay-slot pairs split the function), every RAW / WAR / barrier
/// pair keeps its order, a WAW pair keeps its order when the later write is
/// observed by a read, and the $acc-touching subsequence is a concatenation
/// of intact chains. Offset-rebase hops across the matching increment are
/// allowed. On failure returns false and describes the first violation in
/// `err` (if given).
bool compactVerifySeq(const CompactFunc &cf, const std::vector<int> &seq,
                      std::string *err = nullptr);

} // namespace rspl

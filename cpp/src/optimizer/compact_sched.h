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
// The operations here are exact ports of asmGetReorderIndices,
// relocateElement, asmTryRebaseCross and evalFunctionCost; the tests assert
// equality against those on random orders.
#include "../asm.h"

#include <array>
#include <cstdint>
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

struct CompactFunc {
  std::vector<CompactOp> ops;   // item id -> data (last entry: the NOP)
  int nopId = -1;
  std::vector<AsmInst> orig;    // item id -> original instruction (apply)
};

struct CompactState {
  std::vector<int> seq;            // item ids in program order
  std::vector<int32_t> rebaseValue; // per item id (mem ops)
  std::vector<int16_t> stall;      // per item id: eval feedback for heuristics
  std::vector<uint8_t> paired;     // per item id
  std::vector<int16_t> reorderCount; // per item id (debug)
  int hotCycles = 0;
};

CompactFunc compactBuild(const AsmFunc &func);
CompactState compactInitialState(const CompactFunc &cf);

/// Positions the item at `pos` may legally occupy (port of asmGetReorderIndices)
std::vector<int> compactReorderIndices(const CompactFunc &cf, const CompactState &st, int pos);
/// Move item from `from` to `to` (port of relocateElement, incl. NOP handling)
void compactRelocate(const CompactFunc &cf, CompactState &st, int from, int to);
/// Offset-rebase hop (port of asmTryRebaseCross)
bool compactTryRebaseCross(const CompactFunc &cf, CompactState &st, int pos, bool forward);
/// Weighted path-aware cost (port of evalFunctionCost); sets st.hotCycles
/// and the per-op stall/paired feedback
int compactEvalCost(const CompactFunc &cf, CompactState &st);
/// Materialize the state back into func.asm_ (rebased offsets rewritten,
/// dependency data re-initialized)
void compactApply(const CompactFunc &cf, const CompactState &st, AsmFunc &func);

} // namespace rspl

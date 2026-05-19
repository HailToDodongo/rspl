#include "astCalcNormalizer.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace rspl {

void applyPrecedence(std::vector<FlatElem> &parts, int level) {
  static const std::vector<std::vector<std::string>> precedence = {
      {"*", "/"},
      {"+", "-"},
      {"<<", ">>", ">>>"},
      {"&"},
      {"^"},
      {"|"},
  };

  for (const auto &ops : precedence) {
    int idx = -1;
    for (size_t i = 0; i <= parts.size(); i++) {
      // sentinel for past-the-end (JS: parts[i] || " ")
      if (i == parts.size()) {
        if (idx >= 0) {
          if (i != parts.size() - 1) {
            std::vector<FlatElem> sub(parts.begin() + idx,
                                      parts.begin() + i);
            FlatElem nested;
            nested.kind = FlatElem::VAL;
            nested.varName = NESTED_SENTINEL;
            nested.nested = std::move(sub);
            nested.isNested = true;
            parts.erase(parts.begin() + idx, parts.begin() + i);
            parts.insert(parts.begin() + idx, std::move(nested));
            i = idx + 1;
          }
          idx = -1;
        }
        break;
      }

      FlatElem &part = parts[i];

      // non-string in JS → non-OP in C++
      if (part.kind != FlatElem::OP) {
        if (part.isNested && level == 0)
          applyPrecedence(part.nested, level + 1);
        continue;
      }

      bool isPrecOp =
          std::find(ops.begin(), ops.end(), part.opStr) != ops.end();
      if (idx == -1 && isPrecOp) {
        idx = static_cast<int>(i) - 1;
      }
      if (idx >= 0 && !isPrecOp) {
        if (i != parts.size() - 1) {
          std::vector<FlatElem> sub(parts.begin() + idx,
                                    parts.begin() + i);
          FlatElem nested;
          nested.kind = FlatElem::VAL;
          nested.varName = NESTED_SENTINEL;
          nested.nested = std::move(sub);
          nested.isNested = true;
          parts.erase(parts.begin() + idx, parts.begin() + i);
          parts.insert(parts.begin() + idx, std::move(nested));
          i = idx + 1;
        }
        idx = -1;
      }
    }
  }
}

PartsResult partsEval(std::vector<FlatElem> &parts, int level) {
  for (size_t i = 0; i < parts.size(); i++) {
    if (parts[i].isNested) {
      auto nestedResult = partsEval(parts[i].nested, level + 1);
      if (std::holds_alternative<FlatElem>(nestedResult)) {
        // bracket was completely evaluated into single value
        parts[i] = std::get<FlatElem>(std::move(nestedResult));
        i = static_cast<size_t>(-1); // restart
      } else {
        parts[i].nested =
            std::get<std::vector<FlatElem>>(std::move(nestedResult));
      }
    } else if (parts[i].kind == FlatElem::VAL) {
      // if both sides are (unswizzled) numbers, we can evaluate them
      if (i + 2 >= parts.size()) continue;
      if (parts[i + 1].kind != FlatElem::OP) continue;
      if (parts[i + 2].kind != FlatElem::VAL) continue;
      if (!parts[i].swizzle.empty() || !parts[i + 2].swizzle.empty())
        continue;
      if (!parts[i].isNum || !parts[i + 2].isNum) continue;

      double valueL = parts[i].numVal;
      double valueR = parts[i + 2].numVal;
      std::string op = parts[i + 1].opStr;

      double newVal;
      bool ok = true;
      if (op == "+") newVal = valueL + valueR;
      else if (op == "-") newVal = valueL - valueR;
      else if (op == "*") newVal = valueL * valueR;
      else if (op == "/") newVal = valueL / valueR;
      else if (op == "<<")
        newVal = static_cast<int64_t>(valueL) << static_cast<int>(valueR);
      else if (op == ">>")
        newVal = static_cast<int64_t>(valueL) >> static_cast<int>(valueR);
      else if (op == ">>>") {
        uint32_t u = static_cast<uint32_t>(valueL);
        u >>= static_cast<int>(valueR);
        newVal = static_cast<double>(u);
      } else if (op == "&")
        newVal = static_cast<int64_t>(valueL) & static_cast<int64_t>(valueR);
      else if (op == "^")
        newVal = static_cast<int64_t>(valueL) ^ static_cast<int64_t>(valueR);
      else if (op == "|")
        newVal = static_cast<int64_t>(valueL) | static_cast<int64_t>(valueR);
      else
        ok = false;

      if (ok) {
        // replace the 3 parts with the computed value
        FlatElem folded;
        folded.kind = FlatElem::VAL;
        folded.isNum = true;
        folded.numVal = newVal;
        parts.erase(parts.begin() + i, parts.begin() + i + 3);
        parts.insert(parts.begin() + i, std::move(folded));
        i--; // re-check this position
      }
    }
  }

  if (parts.size() == 1) return parts[0];
  return parts;
}

} // namespace rspl

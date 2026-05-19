#pragma once

#include <string>
#include <variant>
#include <vector>

namespace rspl {

// A single element in the expression tree.  Mirrors the JS representation
// where each element is either an operator string, a value object, or a
// nested sub-array.
struct FlatElem {
  enum Kind { VAL, OP };
  Kind kind;
  std::string opStr;        // for OP ("+", "-", "*", "<<", "&", etc.)
  double numVal = 0;        // for VAL when numeric
  std::string varName;      // for VAL when variable
  std::string swizzle;      // swizzle on the value
  bool isNum = false;       // VAL is numeric
  std::vector<FlatElem> nested; // nested sub-expression
  bool isNested = false;    // true when wrapping a sub-expression
};

inline const char NESTED_SENTINEL[] = "\x01";

using PartsResult = std::variant<FlatElem, std::vector<FlatElem>>;

// Evaluates constant sub-expressions at compile time.
// Returns a single FlatElem if the entire expression folded to a constant,
// or the (possibly modified) parts vector.
PartsResult partsEval(std::vector<FlatElem> &parts, int level = 0);

// Applies order-of-operations by nesting higher-precedence operators
// into sub-arrays.  E.g. [a, "+", b, "*", c] -> [a, "+", [b, "*", c]].
void applyPrecedence(std::vector<FlatElem> &parts, int level = 0);

} // namespace rspl

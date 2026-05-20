#pragma once
#include <sstream>
#include <string>
#include <vector>

// Print up to `maxDiffs` line-by-line differences with `ctx` surrounding
// context lines.  Shown as "- expected" / "+ actual" pairs.
inline std::string diffLines(const std::string &expected,
                             const std::string &actual, int ctx = 2,
                             int maxDiffs = 25) {
  std::vector<std::string> e, a;
  auto split = [](const std::string &s, std::vector<std::string> &v) {
    std::istringstream ss(s);
    std::string l;
    while (std::getline(ss, l)) v.push_back(l);
  };
  split(expected, e);
  split(actual, a);

  // Simple equal-prefix walk, then show context around mismatches.
  size_t i = 0;
  int shown = 0;
  std::ostringstream out;
  while (i < e.size() || i < a.size()) {
    bool eq =
        i < e.size() && i < a.size() && e[i] == a[i];
    if (eq) { ++i; continue; }
    if (shown >= maxDiffs) break;

    // Print context before
    size_t cs = (i > (size_t)ctx) ? i - ctx : 0;
    if (shown > 0) out << "--\n";
    out << "Line " << (i + 1) << ":\n";
    for (size_t k = cs; k < i && k < e.size(); ++k)
      out << "  " << e[k] << "\n";
    // Print diff
    if (i < e.size()) out << "- " << e[i] << "\n";
    if (i < a.size()) out << "+ " << a[i] << "\n";
    // Print context after
    size_t ce = std::min(i + ctx + 1,
                         std::max(e.size(), a.size()));
    for (size_t k = i + 1; k < ce; ++k) {
      if (k < e.size() && k < a.size() && e[k] == a[k])
        out << "  " << e[k] << "\n";
      else
        break;
    }
    ++i; ++shown;
  }
  if (shown >= maxDiffs)
    out << "... (" << maxDiffs << " diffs shown, more omitted)\n";

  // If no diffs found but lengths differ, show tail
  if (shown == 0 && e.size() != a.size()) {
    out << "Length mismatch: expected " << e.size() << " lines, got "
        << a.size() << "\n";
    size_t start = std::min(e.size(), a.size());
    for (size_t k = start; k < std::min(start + 5, e.size()); ++k)
      out << "- " << e[k] << "\n";
    for (size_t k = start; k < std::min(start + 5, a.size()); ++k)
      out << "+ " << a[k] << "\n";
  }
  return out.str();
}

#define REQUIRE_ASM_EQ(expected, actual)                                     \
  do {                                                                       \
    if ((expected) != (actual)) {                                            \
      FAIL_CHECK("ASM mismatch:\n" << diffLines(expected, actual));          \
    }                                                                        \
  } while (0)

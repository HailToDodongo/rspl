#pragma once

namespace rspl {

struct AsmFunc;

/// Remove instructions that write to $zero or $vzero (dead writes).
void normalizeASM(AsmFunc &func);

} // namespace rspl

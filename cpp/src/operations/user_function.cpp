#include "user_function.h"
#include "scalar.h"

#include "../asm.h"
#include "../state.h"

namespace rspl::ops {

std::vector<AsmInst> callUserFunction(
    const std::string &name, const std::vector<ast::FuncArg> &args) {
  const FuncDef *userFunc = state.getFunction(name);
  if (!userFunc) {
    if (!state.varExists(name)) {
      state.throwError("Function " + name + " not known!");
    }
    // Indirect call through register variable
    static FuncDef indirect;
    indirect.name = *state.getVarReg(name);
    indirect.isRelative = false;
    userFunc = &indirect;
  }

  std::vector<AsmInst> res;

  if (userFunc->args.size() != args.size()) {
    state.throwError("Function " + name + " expects " +
                     std::to_string(userFunc->args.size()) +
                     " arguments, got " +
                     std::to_string(args.size()) + "!");
  }

  for (size_t i = 0; i < args.size(); ++i) {
    const auto &argUser = args[i];
    const auto &argDef = userFunc->args[i];
    if (argUser.type == "num") {
      auto load =
          loadImmediate(argDef.reg, argUser.value);
      res.insert(res.end(), load.begin(), load.end());
    } else {
      const VarDef *argVar =
          state.getRequiredVar(argUser.value, "arg" + std::to_string(i));
      if (argVar->type != argDef.type) {
        state.throwError("Function " + name +
                         " expects argument " + std::to_string(i) +
                         " to be of type " + argDef.type +
                         ", got " + argVar->type + "!");
      }
      if (argVar->reg != argDef.reg) {
        state.throwError("Function " + name +
                         " expects argument " + std::to_string(i) +
                         " to be in register " + argDef.reg +
                         ", got " + argVar->reg + "!");
      }
    }
  }

  bool isRelative = userFunc->isRelative;
  auto annos = state.getAnnotations("Relative");
  if (!annos.empty()) isRelative = true;

  std::vector<std::string> regsArg;
  for (const auto &arg : userFunc->args) {
    regsArg.push_back(arg.reg);
  }

  res.push_back(asmFunction(userFunc->name, regsArg, isRelative));
  res.push_back(asmNOP());
  return res;
}

} // namespace rspl::ops

#include "builtins.h"
#include "asm.h"
#include "operations/branch.h"
#include "operations/scalar.h"
#include "operations/user_function.h"
#include "operations/vector.h"
#include "registers.h"
#include "state.h"
#include "swizzle.h"
#include "types.h"

#include <unordered_map>

namespace rspl::builtins {

using namespace rspl::ops;

static const std::string LABEL_ASSERT = "assertion_failed";

static const std::unordered_map<std::string, int64_t> DMA_FLAGS = {
    {"DMA_IN_ASYNC", 0x00000000},
    {"DMA_OUT_ASYNC", 0xFFFF8000},
    {"DMA_IN", 0x00000000},       // + busy/full flags in real code
    {"DMA_OUT", 0xFFFF8000},
};

// --- Helpers ----------------------------------------------------------

static void assertArgsNoSwizzle(const std::vector<ast::FuncArg> &args,
                                int offset = 0) {
  for (size_t i = offset; i < args.size(); ++i) {
    if (!args[i].swizzle.empty()) {
      state.throwError(
          offset > 0
              ? "Only the first " + std::to_string(offset) +
                    " argument(s) can use swizzling!"
              : "Arguments with swizzle not allowed in this function!");
    }
  }
}

static VarDef resolveArg(const ast::FuncArg &arg,
                         const std::string &ctx) {
  if (arg.type == "num") {
    VarDef v;
    v.value = std::stoll(arg.value);
    return v;
  }
  // Try register variable first, fall back to memory variable
  if (state.varExists(arg.value)) {
    VarDef v = state.getRequiredVarCopy(arg.value, ctx);
    v.swizzle = arg.swizzle;
    return v;
  }
  auto memVar = state.getRequiredVarOrMem(arg.value, ctx);
  VarDef v;
  v.type = memVar.type;
  v.reg = ""; // no register — it's a memory label
  v.swizzle = arg.swizzle;
  v.value = 0;
  v.name = memVar.name; // store name for label reference
  return v;
}

// --- Builtin implementations ------------------------------------------

// load()
static std::vector<AsmInst>
b_load(const VarDef *varRes, const std::vector<ast::FuncArg> &args,
       const std::string &swizzle) {
  assertArgsNoSwizzle(args);
  if (!varRes)
    state.throwError("Builtin load() needs a left-side");
  if (args.empty())
    state.throwError("Builtin load() requires at least 1 argument!");

  auto argVar = state.getRequiredVarOrMem(args[0].value, "arg0");
  VarOrMem argOffset;
  if (args.size() >= 2 && args[1].type == "num")
    argOffset.reg = args[1].value;
  else if (args.size() >= 2)
    argOffset = state.getRequiredVarOrMem(args[1].value, "arg1");

  if (!argVar.reg.empty() && isVecType(argVar.type))
    state.throwError(
        "Builtin load() requires first argument to be a scalar!");

  if (reg::isVecReg(varRes->reg)) {
    return opLoadVec(*varRes, argVar, argOffset, swizzle);
  }
  return opLoad(*varRes, argVar, argOffset);
}

// load variant with extra flags
static std::vector<AsmInst>
b_load_ex(const VarDef *varRes, const std::vector<ast::FuncArg> &args,
          const std::string &swizzle,
          bool isPackedByte, bool isSigned, bool isUnaligned) {
  assertArgsNoSwizzle(args);
  if (!varRes) state.throwError("Builtin load() needs a left-side");
  if (args.empty()) state.throwError("Builtin load() requires at least 1 argument!");
  auto argVar = state.getRequiredVarOrMem(args[0].value, "arg0");
  VarOrMem argOffset;
  if (args.size() >= 2 && args[1].type == "num")
    argOffset.reg = args[1].value;
  else if (args.size() >= 2)
    argOffset = state.getRequiredVarOrMem(args[1].value, "arg1");
  if (!argVar.reg.empty() && isVecType(argVar.type))
    state.throwError("Builtin load() requires first argument to be a scalar!");
  if (reg::isVecReg(varRes->reg))
    return opLoadVec(*varRes, argVar, argOffset, swizzle, isPackedByte, isSigned, isUnaligned);
  return opLoad(*varRes, argVar, argOffset);
}

// store()
static std::vector<AsmInst>
b_store(const VarDef *varRes, const std::vector<ast::FuncArg> &args,
        const std::string &swizzle) {
  assertArgsNoSwizzle(args, 1);
  if (varRes)
    state.throwError("Builtin store() cannot have a left side!");
  if (args.empty() || args[0].type != "var")
    state.throwError("Builtin store() requires first argument to be a variable!");

  VarDef varSrc = resolveArg(args[0], "arg0");
  varSrc.swizzle = args[0].swizzle;

  std::vector<VarOrMem> offsets;
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i].type == "num") {
      VarOrMem v;
      v.reg = args[i].value;
      offsets.push_back(v);
    } else {
      offsets.push_back(state.getRequiredVarOrMem(args[i].value, "store_offset"));
    }
  }

  if (reg::isVecReg(varSrc.reg)) {
    return opStoreVec(varSrc, offsets);
  }
  if (!varSrc.swizzle.empty())
    state.throwError("Scalar variables cannot use swizzling!");
  return opStore(varSrc, offsets);
}

// store variant with extra flags
static std::vector<AsmInst>
b_store_ex(const VarDef *varRes, const std::vector<ast::FuncArg> &args,
           const std::string &swizzle,
           bool isPackedByte, bool isSigned, bool isUnaligned) {
  assertArgsNoSwizzle(args, 1);
  if (varRes) state.throwError("Builtin store() cannot have a left side!");
  if (args.empty() || args[0].type != "var")
    state.throwError("Builtin store() requires first argument to be a variable!");
  VarDef varSrc = resolveArg(args[0], "arg0");
  varSrc.swizzle = args[0].swizzle;
  std::vector<VarOrMem> offsets;
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i].type == "num") {
      VarOrMem v; v.reg = args[i].value; offsets.push_back(v);
    } else {
      offsets.push_back(state.getRequiredVarOrMem(args[i].value, "store_offset"));
    }
  }
  if (reg::isVecReg(varSrc.reg))
    return opStoreVec(varSrc, offsets, isPackedByte, isSigned, isUnaligned);
  if (!varSrc.swizzle.empty())
    state.throwError("Scalar variables cannot use swizzling!");
  return opStore(varSrc, offsets);
}

// abs()
static std::vector<AsmInst>
b_abs(const VarDef *varRes, const std::vector<ast::FuncArg> &args,
      const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin abs() cannot use swizzle!");
  if (args.size() != 1)
    state.throwError("Builtin abs() requires exactly one argument!");
  VarDef varArg = resolveArg(args[0], "arg0");
  if (!isVecType(varArg.type))
    state.throwError("Builtin abs() requires a vector argument!");
  if (!varRes)
    state.throwError("Builtin abs() needs a left-side");

  return {asmOp("vabs", {varRes->reg, varArg.reg, varArg.reg})};
}

// clear_vcc()
static std::vector<AsmInst>
b_clear_vcc(const VarDef *varRes,
            const std::vector<ast::FuncArg> &args,
            const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin clear_vcc() cannot use swizzle!");
  if (varRes)
    state.throwError("Builtin clear_vcc() must not have a left side!");
  return {asmOp("vsubc", {reg::Reg::VTEMP0, reg::Reg::VZERO,
                          reg::Reg::VZERO})};
}

// get_vcc()
static std::vector<AsmInst>
b_get_vcc(const VarDef *varRes, const std::vector<ast::FuncArg> &args,
          const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin get_vcc() cannot use swizzle!");
  if (!varRes)
    state.throwError("Builtin get_vcc() must have a left side!");
  return {asmOp("cfc2", {varRes->reg, reg::RegCop2::VCC})};
}

// set_vcc()
static std::vector<AsmInst>
b_set_vcc(const VarDef *varRes, const std::vector<ast::FuncArg> &args,
          const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin set_vcc() cannot use swizzle!");
  if (varRes)
    state.throwError("Builtin set_vcc() must not have a left side!");
  if (args.size() != 1)
    state.throwError("Builtin set_vcc() requires 1 scalar argument!");

  std::string reg = reg::Reg::AT;
  std::vector<AsmInst> res;
  if (args[0].type == "num") {
    auto load = loadImmediate(reg, args[0].value);
    res.insert(res.end(), load.begin(), load.end());
  } else {
    VarDef varArg = resolveArg(args[0], "arg0");
    if (isVecType(varArg.type))
      state.throwError(
          "Builtin set_vcc() requires a scalar argument!");
    reg = varArg.reg;
  }
  res.push_back(asmOp("ctc2", {reg, reg::RegCop2::VCC}));
  return res;
}

// get_acc() -> vec32
static std::vector<AsmInst>
b_get_acc(const VarDef *varRes, const std::vector<ast::FuncArg> &args,
          const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin get_acc() cannot use swizzle!");
  if (!varRes)
    state.throwError("Builtin get_acc() must have a left side!");
  if (!reg::isVecReg(varRes->reg))
    state.throwError(
        "Builtin get_acc() must be assigned to a vector variable!");
  return {asmOp("vsar", {varRes->reg, reg::RegCop2::ACC_HI}),
          asmOp("vsar",
                 {*reg::nextVecReg(varRes->reg), reg::RegCop2::ACC_MD})};
}

// get_acc_high/mid/low
static std::vector<AsmInst>
b_get_acc_part(const VarDef *varRes,
               const std::vector<ast::FuncArg> &args,
               const std::string &swizzle, const std::string &part) {
  if (!swizzle.empty())
    state.throwError("Builtin get_acc_*() cannot use swizzle!");
  if (!varRes)
    state.throwError("Builtin get_acc_*() must have a left side!");
  return {asmOp("vsar", {varRes->reg, part})};
}

// mfc0 reads (generic)
static std::vector<AsmInst>
b_mfc0_read(const VarDef *varRes, const std::string &rdpReg) {
  if (!varRes)
    state.throwError("Builtin requires a left side!");
  return {asmOp("mfc0", {varRes->reg, rdpReg})};
}

// mtc0 writes (generic)
static std::vector<AsmInst>
b_mtc0_write(const VarDef *varRes,
             const std::vector<ast::FuncArg> &args,
             const std::string &rdpReg) {
  if (varRes)
    state.throwError("Builtin must not have a left side!");
  std::string reg = reg::Reg::AT;
  std::vector<AsmInst> res;
  if (args[0].type == "num") {
    auto load = loadImmediate(reg, args[0].value);
    res.insert(res.end(), load.begin(), load.end());
  } else {
    VarDef varArg = resolveArg(args[0], "arg0");
    reg = varArg.reg;
  }
  res.push_back(asmOp("mtc0", {reg, rdpReg}));
  return res;
}

// get_cmd_address()
static std::vector<AsmInst>
b_get_cmd_address(const VarDef *varRes,
                  const std::vector<ast::FuncArg> &args,
                  const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError(
        "Builtin get_cmd_address() cannot use swizzle!");
  if (!varRes)
    state.throwError(
        "Builtin get_cmd_address() must have a left side!");
  int offset = args.empty() ? 0 : std::stoi(args[0].value);
  offset -= state.argSize;
  std::string offStr = "%lo(RSPQ_DMEM_BUFFER)";
  if (offset < 0) offStr += " - " + std::to_string(-offset);
  else if (offset > 0) offStr += " + " + std::to_string(offset);
  return {asmOp("addiu", {varRes->reg, reg::Reg::GP, offStr})};
}

// load_arg()
static std::vector<AsmInst>
b_load_arg(const VarDef *varRes,
           const std::vector<ast::FuncArg> &args,
           const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin load_arg() cannot use swizzle!");
  if (!varRes)
    state.throwError("Builtin load_arg() must have a left side!");
  int offset = args.empty() ? 0 : std::stoi(args[0].value);
  offset -= state.argSize;
  VarOrMem loc;
  loc.reg = reg::Reg::GP;
  VarOrMem off;
  off.reg = std::string("RSPQ_DMEM_BUFFER") +
            (offset < 0 ? " - " : " + ") +
            std::to_string(std::abs(offset));
  return opLoad(*varRes, loc, off);
}

// dma_in / dma_out / dma_in_async / dma_out_async
static std::vector<AsmInst>
b_dma(const VarDef *varRes,
      const std::vector<ast::FuncArg> &args,
      const std::string &swizzle, const std::string &builtinName,
      const std::string &dmaName) {
  assertArgsNoSwizzle(args);
  if (!swizzle.empty())
    state.throwError("Builtin " + builtinName +
                     "() cannot use swizzle!");
  if (varRes)
    state.throwError("Builtin " + builtinName +
                     "() cannot have a left side!");

  auto targetMem = state.getRequiredVarOrMem(args[0].value, "dest");
  VarDef varRDRAM = resolveArg(args[1], "RDRAM");

  // Build the DMA call — simplified: emit DMAExec call
  std::vector<AsmInst> res;
  if (varRDRAM.reg != reg::Reg::S0) {
    res.push_back(asmOp("or",
                        {reg::Reg::S0, reg::Reg::ZERO, varRDRAM.reg}));
  }
  auto load =
      loadImmediate(reg::Reg::S4, "%lo(" + targetMem.name + ")");
  res.insert(res.end(), load.begin(), load.end());
  auto flagsIt = DMA_FLAGS.find(dmaName);
  auto loadflags = loadImmediate(reg::Reg::T2,
      std::to_string(flagsIt != DMA_FLAGS.end() ? flagsIt->second
                                                 : 0));
  res.insert(res.end(), loadflags.begin(), loadflags.end());
  res.push_back(
      asmFunction("DMAExec",
                  {reg::Reg::T0, reg::Reg::T1, reg::Reg::S0,
                   reg::Reg::S4, reg::Reg::T2}));
  res.push_back(asmNOP());
  return res;
}

// dma_await()
static std::vector<AsmInst>
b_dma_await(const VarDef *varRes,
            const std::vector<ast::FuncArg> &args,
            const std::string &swizzle) {
  if (varRes)
    state.throwError(
        "Builtin dma_await() cannot have a left side!");
  return {asmFunction("DMAWaitIdle", {}), asmNOP()};
}

// swap()
static std::vector<AsmInst>
b_swap(const VarDef *varRes,
       const std::vector<ast::FuncArg> &args,
       const std::string &swizzle) {
  assertArgsNoSwizzle(args);
  if (!swizzle.empty())
    state.throwError("Builtin swap() cannot use swizzle!");
  if (varRes)
    state.throwError("Builtin swap() cannot have a left side!");
  if (args.size() != 2)
    state.throwError(
        "Builtin swap() requires exactly two arguments!");

  VarDef varA = resolveArg(args[0], "arg0");
  VarDef varB = resolveArg(args[1], "arg1");

  if (varA.reg == varB.reg) return {};

  std::string op = isVecType(varA.type) ? "vxor" : "xor";
  std::vector<AsmInst> res;
  res.push_back(asmOp(op, {varA.reg, varA.reg, varB.reg}));
  res.push_back(asmOp(op, {varB.reg, varA.reg, varB.reg}));
  res.push_back(asmOp(op, {varA.reg, varA.reg, varB.reg}));

  if (isTwoRegType(varA.type)) {
    std::string ra = *reg::nextReg(varA.reg);
    std::string rb = *reg::nextReg(varB.reg);
    res.push_back(asmOp(op, {ra, ra, rb}));
    res.push_back(asmOp(op, {rb, ra, rb}));
    res.push_back(asmOp(op, {ra, ra, rb}));
  }
  return res;
}

// min / max
static std::vector<AsmInst>
b_minmax(const VarDef *varRes,
         const std::vector<ast::FuncArg> &args,
         const std::string &swizzle, const std::string &compareOp) {
  if (!swizzle.empty())
    state.throwError("Builtin min/max() cannot use swizzle!");
  if (args.size() != 2)
    state.throwError(
        "Builtin min/max() requires exactly two arguments!");

  VarDef varA = resolveArg(args[0], "arg0");
  VarDef varB = resolveArg(args[1], "arg1");
  if (!varRes)
    state.throwError("Builtin min/max() needs a left-side");

  return opCompareVec(*varRes, varA, varB, compareOp, nullptr);
}

// invert_half / invert_half_sqrt
static std::vector<AsmInst>
b_invert_half(const VarDef *varRes,
              const std::vector<ast::FuncArg> &args,
              const std::string &swizzle) {
  assertArgsNoSwizzle(args);
  if (args.size() != 1)
    state.throwError(
        "Builtin invert_half() requires exactly one argument!");
  VarDef varArg = resolveArg(args[0], "arg0");
  if (!isVecType(varArg.type))
    state.throwError(
        "Builtin invert_half() requires a vector argument!");
  if (!varRes)
    state.throwError("Builtin invert_half() needs a left-side");

  VarDef resSwiz = *varRes;
  resSwiz.swizzle = swizzle;
  VarDef argSwiz = varArg;
  argSwiz.swizzle = swizzle;
  return opInvertHalf(resSwiz, argSwiz);
}

static std::vector<AsmInst>
b_invert(const VarDef *varRes,
         const std::vector<ast::FuncArg> &args,
         const std::string &swizzle) {
  // invert = invert_half + multiply by 2
  if (swizzle.size())
    state.throwError(
        "Builtin invert() cannot use swizzle, use invert_half() instead");
  auto res = b_invert_half(varRes, args, swizzle);
  VarDef mulRight;
  mulRight.value = 2;
  auto mulAsm = opMulVec(*varRes, *varRes, mulRight, true);
  res.insert(res.end(), mulAsm.begin(), mulAsm.end());
  return res;
}

static std::vector<AsmInst>
b_invert_half_sqrt(const VarDef *varRes,
                   const std::vector<ast::FuncArg> &args,
                   const std::string &swizzle) {
  assertArgsNoSwizzle(args);
  if (args.size() != 1)
    state.throwError(
        "Builtin invert_half_sqrt() requires exactly one argument!");
  VarDef varArg = resolveArg(args[0], "arg0");
  if (!isVecType(varArg.type))
    state.throwError(
        "Builtin invert_half_sqrt() requires a vector argument!");
  if (!varRes)
    state.throwError(
        "Builtin invert_half_sqrt() needs a left-side");

  VarDef resSwiz = *varRes;
  resSwiz.swizzle = swizzle;
  VarDef argSwiz = varArg;
  argSwiz.swizzle = swizzle;
  return opInvertSqrtHalf(resSwiz, argSwiz);
}

// select()
static std::vector<AsmInst>
b_select(const VarDef *varRes,
         const std::vector<ast::FuncArg> &args,
         const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError(
        "Builtin select() cannot use swizzle!");
  if (args.size() != 2)
    state.throwError(
        "Builtin select() requires exactly two arguments!");

  VarDef varLeft, varRight;
  if (args[0].type == "num") {
    varLeft.reg = reg::Reg::VZERO;
    varLeft.type = "vec16";
  } else {
    varLeft = resolveArg(args[0], "arg0");
  }
  if (args[1].type == "num") {
    auto pIt = POW2_SWIZZLE_VAR.find(std::stoll(args[1].value));
    if (pIt == POW2_SWIZZLE_VAR.end())
      state.throwError(
          "Second arg must be a variable or power-of-two constant!");
    varRight.reg = pIt->second.reg;
    varRight.swizzle = pIt->second.swizzle;
    varRight.type = "vec16";
  } else {
    varRight = resolveArg(args[1], "arg1");
  }
  if (!varRes)
    state.throwError("Builtin select() needs a left-side");

  auto regsDst = ops::getVec32Regs(*varRes);
  auto regsL = ops::getVec32Regs(varLeft);
  auto regsR = ops::getVec32Regs(varRight);
  if (!isTwoRegType(varRes->type)) {
    regsL.first = varLeft.reg;
    regsR.first = varRight.reg;
  }
  auto sit = SWIZZLE_MAP.find(varRight.swizzle);
  std::string swSuffix =
      sit != SWIZZLE_MAP.end() ? sit->second : "";
  if (swSuffix == ".v") swSuffix = "";

  std::vector<std::string> args1 = {regsDst.first, regsL.first,
                                     regsR.first + swSuffix};
  std::vector<std::string> args2 = {regsDst.second, regsL.second,
                                     regsR.second + swSuffix};
  return {asmOp("vmrg", args1), asmOp("vmrg", args2)};
}

// assert()
static std::vector<AsmInst>
b_assert(const VarDef *varRes,
         const std::vector<ast::FuncArg> &args,
         const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin assert() cannot use swizzle!");
  if (varRes)
    state.throwError("Builtin assert() cannot have a left side!");
  int code = std::stoi(args[0].value);
  return {asmOp("lui", {reg::Reg::AT, std::to_string(code)}),
          asmOp("j", {LABEL_ASSERT}), asmNOP()};
}

// asm() — raw inline assembly
static std::vector<AsmInst>
b_asm(const VarDef *varRes,
      const std::vector<ast::FuncArg> &args,
      const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin asm() cannot use swizzle!");
  if (varRes)
    state.throwError("Builtin asm() cannot have a left side!");
  if (args.empty() || args[0].type != "string")
    state.throwError(
        "Builtin asm() requires the first argument to be a string!");
  return {asmInline(args[0].value, {"# inline-ASM"})};
}

// transpose()
// Valid transpose registers: $v00, $v08, $v16, $v24
static const std::vector<std::string> VALID_TRANSPOSE_REGS = {
    "$v00", "$v08", "$v16", "$v24"};

static std::vector<AsmInst>
b_transpose(const VarDef *varRes,
            const std::vector<ast::FuncArg> &args,
            const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin transpose() cannot use swizzle!");
  if (!varRes)
    state.throwError("Builtin transpose() needs a left-side");
  if (!reg::isVecReg(varRes->reg))
    state.throwError(
        "Builtin transpose() must store the result into a vector!");
  if (args.size() != 4)
    state.throwError("Builtin transpose() requires 4 arguments!");

  VarDef varSrc = resolveArg(args[0], "arg0");
  if (!isVecType(varSrc.type))
    state.throwError(
        "Builtin transpose() requires first argument to be a vector!");
  VarDef buffVar = resolveArg(args[1], "arg1");
  if (isVecType(buffVar.type))
    state.throwError(
        "Builtin transpose() requires second argument to be a scalar!");
  if (args[2].type != "num")
    state.throwError(
        "Builtin transpose() requires third argument to be a number!");
  if (args[3].type != "num")
    state.throwError(
        "Builtin transpose() requires fourth argument to be a number!");

  int dimX = std::stoi(args[2].value);
  int dimY = std::stoi(args[3].value);
  if (dimX < 1 || dimX > 8 || dimY < 1 || dimY > 8)
    state.throwError(
        "Builtin transpose() requires X and Y dimension to be between 1 "
        "and 8!");

  if (std::find(VALID_TRANSPOSE_REGS.begin(),
                VALID_TRANSPOSE_REGS.end(),
                varRes->reg) == VALID_TRANSPOSE_REGS.end())
    state.throwError("Builtin transpose() requires target register to be "
                     "$v00, $v08, $v16 or $v24!");
  if (std::find(VALID_TRANSPOSE_REGS.begin(),
                VALID_TRANSPOSE_REGS.end(),
                varSrc.reg) == VALID_TRANSPOSE_REGS.end())
    state.throwError("Builtin transpose() requires source register to be "
                     "$v00, $v08, $v16 or $v24!");

  bool isInPlace = (varSrc.reg == varRes->reg);
  bool is8x8 = (dimX > 4 || dimY > 4);

  std::string bufReg = buffVar.reg;

  std::vector<AsmInst> res;
  // STV stores
  if (!isInPlace)
    res.push_back(
        asmOp("stv", {varSrc.reg, "0", "0", bufReg}));
  res.push_back(asmOp("stv", {varSrc.reg, "2", "16", bufReg}));
  res.push_back(asmOp("stv", {varSrc.reg, "4", "32", bufReg}));
  res.push_back(asmOp("stv", {varSrc.reg, "6", "48", bufReg}));
  if (is8x8)
    res.push_back(asmOp("stv", {varSrc.reg, "8", "64", bufReg}));
  res.push_back(asmOp("stv", {varSrc.reg, "10", "80", bufReg}));
  res.push_back(asmOp("stv", {varSrc.reg, "12", "96", bufReg}));
  res.push_back(asmOp("stv", {varSrc.reg, "14", "112", bufReg}));

  // LTV loads
  res.push_back(asmOp("ltv", {varRes->reg, "14", "16", bufReg}));
  res.push_back(asmOp("ltv", {varRes->reg, "12", "32", bufReg}));
  res.push_back(asmOp("ltv", {varRes->reg, "10", "48", bufReg}));
  if (is8x8)
    res.push_back(asmOp("ltv", {varRes->reg, "8", "64", bufReg}));
  res.push_back(asmOp("ltv", {varRes->reg, "6", "80", bufReg}));
  res.push_back(asmOp("ltv", {varRes->reg, "4", "96", bufReg}));
  res.push_back(asmOp("ltv", {varRes->reg, "2", "112", bufReg}));
  if (!isInPlace)
    res.push_back(
        asmOp("ltv", {varRes->reg, "0", "0", bufReg}));

  return res;
}

// asm_op()
static std::vector<AsmInst>
b_asm_op(const VarDef *varRes,
         const std::vector<ast::FuncArg> &args,
         const std::string &swizzle) {
  if (!swizzle.empty())
    state.throwError("Builtin asm_op() cannot use swizzle!");
  if (varRes)
    state.throwError("Builtin asm_op() cannot have a left side!");
  if (args.empty() || args[0].type != "string")
    state.throwError(
        "Builtin asm_op() requires the first argument to be a opcode!");

  std::vector<std::string> asmArgs = {args[0].value};
  for (size_t i = 1; i < args.size(); ++i) {
    if (args[i].type == "num") {
      asmArgs.push_back(args[i].value);
    } else {
      const VarDef *v = state.getRequiredVar(args[i].value, "arg");
      auto sit = SWIZZLE_MAP.find(args[i].swizzle);
      asmArgs.push_back(v->reg +
                        (sit != SWIZZLE_MAP.end() ? sit->second : ""));
    }
  }
  return {AsmInst{args[0].value, asmArgs}};
}

// asm_include()
static std::vector<AsmInst>
b_asm_include(const VarDef *varRes,
              const std::vector<ast::FuncArg> &args,
              const std::string &swizzle) {
  if (args.empty() || args[0].type != "string")
    state.throwError("Builtin asm_include() requires a path argument!");

  std::vector<AsmInst> res;
  // Emit #defines for all scalar registers
  for (size_t i = 0; i < reg::REGS_SCALAR.size(); ++i) {
    if (i == 1) continue; // skip $at
    std::string name = reg::REGS_SCALAR[i].substr(1);
    res.push_back(
        asmInline("#define " + name + " $" + std::to_string(i)));
  }
  res.push_back(asmInline(".set at"));
  res.push_back(asmInline(".set macro"));
  res.push_back(
      asmInline("#include \"" + args[0].value + "\""));
  res.push_back(asmInline(".set noreorder"));
  res.push_back(asmInline(".set noat"));
  res.push_back(asmInline(".set nomacro"));
  for (const auto &reg : reg::REGS_SCALAR) {
    res.push_back(asmInline("#undef " + reg.substr(1)));
  }
  return res;
}

// --- Registry ---------------------------------------------------------

using BuiltinMap =
    std::unordered_map<std::string, BuiltinFn>;

static BuiltinMap buildRegistry() {
  BuiltinMap m;

  m["load"] = b_load;
  m["store"] = b_store;
  m["load_vec_u8"] = [](const VarDef *vr,
                       const std::vector<ast::FuncArg> &args,
                       const std::string &swizzle) {
    return b_load_ex(vr, args, swizzle, true, false, false);
  };
  m["load_vec_s8"] = [](const VarDef *vr,
                       const std::vector<ast::FuncArg> &args,
                       const std::string &swizzle) {
    return b_load_ex(vr, args, swizzle, true, true, false);
  };
  m["store_vec_u8"] = [](const VarDef *vr,
                         const std::vector<ast::FuncArg> &args,
                         const std::string &swizzle) {
    return b_store_ex(vr, args, swizzle, true, false, false);
  };
  m["store_vec_s8"] = [](const VarDef *vr,
                         const std::vector<ast::FuncArg> &args,
                         const std::string &swizzle) {
    return b_store_ex(vr, args, swizzle, true, true, false);
  };
  m["load_unaligned"] = [](const VarDef *vr,
                           const std::vector<ast::FuncArg> &args,
                           const std::string &swizzle) {
    return b_load_ex(vr, args, swizzle, false, true, true);
  };
  m["store_unaligned"] = [](const VarDef *vr,
                            const std::vector<ast::FuncArg> &args,
                            const std::string &swizzle) {
    return b_store_ex(vr, args, swizzle, false, true, true);
  };
  m["abs"] = b_abs;
  m["clear_vcc"] = b_clear_vcc;
  m["get_vcc"] = b_get_vcc;
  m["set_vcc"] = b_set_vcc;
  m["get_acc"] = b_get_acc;
  m["swap"] = b_swap;
  m["select"] = b_select;
  m["assert"] = b_assert;
  m["asm"] = b_asm;
  m["get_cmd_address"] = b_get_cmd_address;
  m["load_arg"] = b_load_arg;
  m["dma_await"] = b_dma_await;

  // Stubs for other builtins
  m["load_transposed"] = [](const VarDef *vr,
                            const std::vector<ast::FuncArg> &args,
                            const std::string &swizzle) -> std::vector<AsmInst> {
    if (!swizzle.empty()) state.throwError("Builtin load_transposed() cannot use swizzle!");
    if (!vr) state.throwError("Builtin load_transposed() needs a left-side");
    if (!reg::isVecReg(vr->reg)) state.throwError("Builtin load_transposed() must store the result into a vector!");
    if (args.size() < 2 || args.size() > 3) state.throwError("Builtin load_transposed() requires 2 or 3 arguments!");
    if (args[0].type != "num") state.throwError("Builtin load_transposed() requires first argument to be a number (row offset 0-7)!");
    int row = std::stoi(args[0].value);
    if (row < 0 || row > 7) state.throwError("Builtin load_transposed() requires first argument (row) to be a number between 0 and 7!");
    int offset = 0;
    if (args.size() >= 3) {
      if (args[2].type != "num") state.throwError("Builtin load_transposed() requires third argument to be a number (offset in steps of 0x10)!");
      offset = std::stoi(args[2].value);
      if (offset % 16 != 0) state.throwError("Builtin load_transposed() requires offset to be multiple of 16!");
    }
    auto addrMem = state.getRequiredVarOrMem(args[1].value, "addr");
    if (!addrMem.reg.empty() && reg::isVecReg(addrMem.reg)) state.throwError("Builtin load_transposed() requires second argument to be a scalar variable!");
    // Register must be v00/v08/v16/v24
    std::string reg = vr->reg;
    if (reg != "$v00" && reg != "$v08" && reg != "$v16" && reg != "$v24")
      state.throwError("Builtin load_transposed() requires result register to be $v00, $v08, $v16 or $v24!");
    std::string baseReg = addrMem.reg.empty() ? "$zero" : addrMem.reg;
    std::string offStr = std::to_string(offset);
    if (!addrMem.reg.empty()) {
      return {asmOp("ltv", {reg, std::to_string(row * 2), offStr, baseReg})};
    }
    auto loadAt = loadImmediate("$at", "%lo(" + addrMem.name + ")");
    loadAt.push_back(asmOp("ltv", {reg, std::to_string(row * 2), offStr, "$at"}));
    return loadAt;
  };
  m["store_transposed"] = [](const VarDef *vr,
                              const std::vector<ast::FuncArg> &args,
                              const std::string &swizzle) -> std::vector<AsmInst> {
    if (!swizzle.empty()) state.throwError("Builtin store_transposed() cannot use swizzle!");
    if (vr) state.throwError("Builtin store_transposed() has no left-side");
    if (args.size() < 3 || args.size() > 4) state.throwError("Builtin store_transposed() requires 3 or 4 arguments!");
    // args[0] = value to store, args[1] = row, args[2] = addr, args[3] = offset (optional)
    const VarDef *valVar = state.getRequiredVar(args[0].value, "arg0");
    if (!reg::isVecReg(valVar->reg)) state.throwError("Builtin store_transposed() must target a vector register!");
    if (args[1].type != "num") state.throwError("Builtin store_transposed() requires second argument to be a number (row offset 0-7)!");
    int row = std::stoi(args[1].value);
    if (row < 0 || row > 7) state.throwError("Builtin store_transposed() requires second argument (row) to be a number between 0 and 7!");
    int offset = 0;
    if (args.size() >= 4) {
      if (args[3].type != "num") state.throwError("Builtin store_transposed() requires fourth argument to be a number (offset in steps of 0x10)!");
      offset = std::stoi(args[3].value);
      if (offset % 16 != 0) state.throwError("Builtin store_transposed() requires offset to be multiple of 16!");
    }
    auto addrMem = state.getRequiredVarOrMem(args[2].value, "addr");
    if (!addrMem.reg.empty() && reg::isVecReg(addrMem.reg)) state.throwError("Builtin store_transposed() requires third argument to be a scalar variable!");
    if (valVar->reg != "$v00" && valVar->reg != "$v08" && valVar->reg != "$v16" && valVar->reg != "$v24")
      state.throwError("Builtin store_transposed() requires target register to be $v00, $v08, $v16 or $v24!");
    std::string baseReg = addrMem.reg.empty() ? "$zero" : addrMem.reg;
    std::string offStr = std::to_string(offset);
    if (!addrMem.reg.empty()) {
      return {asmOp("stv", {valVar->reg, std::to_string(row * 2), offStr, baseReg})};
    }
    auto loadAt = loadImmediate("$at", "%lo(" + addrMem.name + ")");
    loadAt.push_back(asmOp("stv", {valVar->reg, std::to_string(row * 2), offStr, "$at"}));
    return loadAt;
  };
  m["transpose"] = b_transpose;
  m["asm_op"] = b_asm_op;
  m["asm_include"] = b_asm_include;

  // get_acc_high/mid/low
  m["get_acc_high"] = [](const VarDef *vr,
                         const std::vector<ast::FuncArg> &a,
                         const std::string &s) {
    return b_get_acc_part(vr, a, s, reg::RegCop2::ACC_HI);
  };
  m["get_acc_mid"] = [](const VarDef *vr,
                        const std::vector<ast::FuncArg> &a,
                        const std::string &s) {
    return b_get_acc_part(vr, a, s, reg::RegCop2::ACC_MD);
  };
  m["get_acc_low"] = [](const VarDef *vr,
                        const std::vector<ast::FuncArg> &a,
                        const std::string &s) {
    return b_get_acc_part(vr, a, s, reg::RegCop2::ACC_LO);
  };

  // MFC0 reads
  auto addMfc0Read = [&](const char *name, const char *rdpReg) {
    m[name] = [name, rdpReg](const VarDef *vr,
                             const std::vector<ast::FuncArg> &a,
                             const std::string &s) -> std::vector<AsmInst> {
      if (!s.empty())
        state.throwError(std::string("Builtin ") + name +
                         "() cannot use swizzle!");
      return b_mfc0_read(vr, rdpReg);
    };
  };
  addMfc0Read("get_dma_busy", reg::RegCop0::DMA_BUSY);
  addMfc0Read("get_rdp_start", reg::RegCop0::DP_START);
  addMfc0Read("get_rdp_end", reg::RegCop0::DP_END);
  addMfc0Read("get_rdp_current", reg::RegCop0::DP_CURRENT);
  addMfc0Read("get_ticks", reg::RegCop0::DP_CLOCK);

  // MTC0 writes
  auto addMtc0Write = [&](const char *name, const char *rdpReg) {
    m[name] = [name, rdpReg](const VarDef *vr,
                             const std::vector<ast::FuncArg> &a,
                             const std::string &s) -> std::vector<AsmInst> {
      if (!s.empty())
        state.throwError(std::string("Builtin ") + name +
                         "() cannot use swizzle!");
      return b_mtc0_write(vr, a, rdpReg);
    };
  };
  addMtc0Write("set_rdp_start", reg::RegCop0::DP_START);
  addMtc0Write("set_rdp_end", reg::RegCop0::DP_END);
  addMtc0Write("set_rdp_current", reg::RegCop0::DP_CURRENT);
  addMtc0Write("set_dma_addr_rsp", reg::RegCop0::DMA_SPADDR);
  addMtc0Write("set_dma_addr_rdram", reg::RegCop0::DMA_RAMADDR);
  addMtc0Write("set_dma_write", reg::RegCop0::DMA_WRITE);
  addMtc0Write("set_dma_read", reg::RegCop0::DMA_READ);

  // DMA
  auto addDma = [&](const char *name, const char *dmaFlag) {
    m[name] = [name, dmaFlag](const VarDef *vr,
                              const std::vector<ast::FuncArg> &a,
                              const std::string &s) {
      return b_dma(vr, a, s, name, dmaFlag);
    };
  };
  addDma("dma_in", "DMA_IN");
  addDma("dma_out", "DMA_OUT");
  addDma("dma_in_async", "DMA_IN_ASYNC");
  addDma("dma_out_async", "DMA_OUT_ASYNC");

  // invert
  m["invert_half"] = b_invert_half;
  m["invert"] = b_invert;
  m["invert_half_sqrt"] = b_invert_half_sqrt;

  auto noop = []() -> BuiltinFn {
    return [](const VarDef *,
              const std::vector<ast::FuncArg> &,
              const std::string &) -> std::vector<AsmInst> {
      return {};
    };
  };
  m["set_rsp_status"] = noop();
  m["clip"] = noop();
  m["print"] = noop();
  m["printf"] = noop();
  m["min"] = [](const VarDef *vr,
                const std::vector<ast::FuncArg> &a,
                const std::string &s) { return b_minmax(vr, a, s, "<"); };
  m["max"] = [](const VarDef *vr,
                const std::vector<ast::FuncArg> &a,
                const std::string &s) {
    return b_minmax(vr, a, s, ">=");
  };

  return m;
}

static BuiltinMap registry = buildRegistry();

const BuiltinFn *lookup(const std::string &name) {
  auto it = registry.find(name);
  return it != registry.end() ? &it->second : nullptr;
}

} // namespace rspl::builtins

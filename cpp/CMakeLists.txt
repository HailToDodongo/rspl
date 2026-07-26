cmake_minimum_required(VERSION 3.20)
project(rspl VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Dependencies via FetchContent
include(FetchContent)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

# --- Library sources ------------------------------------------------------

set(RSPL_SOURCES
  src/asm.cpp
  src/asm.h
  src/asm_normalize.cpp
  src/asm_normalize.h
  src/asm_writer.cpp
  src/asm_writer.h
  src/annotations.cpp
  src/annotations.h
  src/ast.cpp
  src/ast.h
  src/ast2asm.cpp
  src/ast2asm.h
  src/ast_normalize.cpp
  src/ast_normalize.h
  src/astCalcNormalizer.cpp
  src/astCalcNormalizer.h
  src/builtins.cpp
  src/builtins.h
  src/operations/branch.cpp
  src/operations/branch.h
  src/operations/scalar.cpp
  src/operations/scalar.h
  src/operations/user_function.cpp
  src/operations/user_function.h
  src/operations/vector.cpp
  src/operations/vector.h
  src/optimizer/asm_optimizer.cpp
  src/optimizer/asm_optimizer.h
  src/optimizer/asm_scan_deps.cpp
  src/optimizer/asm_scan_deps.h
  src/optimizer/eval_cost.cpp
  src/optimizer/eval_cost.h
  src/pipeline.cpp
  src/pipeline.h
  src/preproc.cpp
  src/preproc.h
  src/registers.cpp
  src/registers.h
  src/state.cpp
  src/state.h
  src/swizzle.cpp
  src/swizzle.h
  src/types.cpp
  src/types.h
)

add_library(rspl_core STATIC ${RSPL_SOURCES})
target_include_directories(rspl_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(rspl_core PUBLIC nlohmann_json::nlohmann_json)

# --- CLI executable -------------------------------------------------------

add_executable(rspl src/main.cpp)
target_link_libraries(rspl PRIVATE rspl_core)

# --- Tests ----------------------------------------------------------------

set(TEST_SOURCES
  tests/test_annotations.cpp
  tests/test_ast.cpp
  tests/test_branchConst.cpp
  tests/test_builtins.cpp
  tests/test_builtinsDebug.cpp
  tests/test_builtinsAll.cpp
  tests/test_branchVar.cpp
  tests/test_branchZero.cpp
  tests/test_compare.cpp
  tests/test_const.cpp
  tests/test_control.cpp
  tests/test_debugInfo.cpp
  tests/test_defineAsm.cpp
  tests/test_dma.cpp
  tests/test_immediateScalar.cpp
  tests/test_labels.cpp
  tests/test_load.cpp
  tests/test_loop.cpp
  tests/test_macros.cpp
  tests/test_magma.cpp
  tests/test_preproc.cpp
  tests/test_scalarOps.cpp
  tests/test_scope.cpp
  tests/test_state.cpp
  tests/test_stateDataBss.cpp
  tests/test_store.cpp
  tests/test_swizzle.cpp
  tests/test_syntaxExpansion.cpp
  tests/test_optAssert.cpp
  tests/test_optBranchJump.cpp
  tests/test_optDeadCode.cpp
  tests/test_optDedupeImm.cpp
  tests/test_optDelaySlot.cpp
  tests/test_optJumpDedupe.cpp
  tests/test_optLabels.cpp
  tests/test_optDepScanCtrl.cpp
  tests/test_optDepScanMem.cpp
  tests/test_optDepScanRegs.cpp
  tests/test_optMergeSequence.cpp
  tests/test_optRegScan.cpp
  tests/test_reorderPartialLoad.cpp
  tests/test_evalCost.cpp
  tests/test_evalCostExample.cpp
  tests/test_examples.cpp
  tests/test_vectorOps.cpp
  tests/test_syntaxNumbers.cpp
  tests/test_syntaxVar.cpp
)

add_executable(rspl_tests ${TEST_SOURCES})
target_link_libraries(rspl_tests PRIVATE rspl_core Catch2::Catch2WithMain)
include(CTest)
add_test(NAME rspl_tests COMMAND rspl_tests)

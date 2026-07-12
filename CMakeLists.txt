cmake_minimum_required(VERSION 3.20)
project(cameff VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(CAMEFF_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(CAMEFF_ENABLE_SANITIZERS "Enable address and undefined-behavior sanitizers" OFF)

add_library(cameff_core STATIC
    src/catalog.c
    src/config.c
    src/evaluation.c
    src/expert_registry.c
    src/fusion.c
    src/experts/baseline_expert.c
    src/experts/subduction_expert.c
    src/experts/crustal_fault_expert.c
    src/experts/cascade_expert.c
    src/geo.c
    src/pattern.c
    src/signals.c
    src/framework.c
)

target_include_directories(cameff_core PUBLIC include)
if(NOT WIN32)
    target_compile_definitions(cameff_core PRIVATE _GNU_SOURCE)
endif()

if(MSVC)
    target_compile_options(cameff_core PRIVATE /W4)
    if(CAMEFF_WARNINGS_AS_ERRORS)
        target_compile_options(cameff_core PRIVATE /WX)
    endif()
else()
    target_compile_options(cameff_core PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes
    )
    if(CAMEFF_WARNINGS_AS_ERRORS)
        target_compile_options(cameff_core PRIVATE -Werror)
    endif()
    target_link_libraries(cameff_core PUBLIC m)
    if(CAMEFF_ENABLE_SANITIZERS)
        target_compile_options(cameff_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(cameff_core PRIVATE -fsanitize=address,undefined)
    endif()
endif()

add_executable(cameff apps/cameff_cli.c)
target_link_libraries(cameff PRIVATE cameff_core)

if(NOT MSVC)
    target_compile_options(cameff PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(CAMEFF_WARNINGS_AS_ERRORS)
        target_compile_options(cameff PRIVATE -Werror)
    endif()
endif()

enable_testing()
add_executable(cameff_tests tests/test_main.c)
target_link_libraries(cameff_tests PRIVATE cameff_core)
if(NOT MSVC)
    target_compile_options(cameff_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    if(CAMEFF_WARNINGS_AS_ERRORS)
        target_compile_options(cameff_tests PRIVATE -Werror)
    endif()
endif()
add_test(NAME cameff-unit-tests COMMAND cameff_tests)
add_executable(cameff_m2_contract_tests tests/test_m2_contracts.c)
target_link_libraries(cameff_m2_contract_tests PRIVATE cameff_core)
if(NOT MSVC)
 target_compile_options(cameff_m2_contract_tests PRIVATE
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow
 )
 if(CAMEFF_WARNINGS_AS_ERRORS)
  target_compile_options(cameff_m2_contract_tests PRIVATE -Werror)
 endif()
endif()
add_test(NAME cameff-m2-contract-tests COMMAND cameff_m2_contract_tests)
add_executable(cameff_baseline_expert_tests
  tests/test_baseline_expert.c
)

target_link_libraries(cameff_baseline_expert_tests
  PRIVATE cameff_core
)

if(NOT MSVC)
  target_compile_options(cameff_baseline_expert_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
  )

  if(CAMEFF_WARNINGS_AS_ERRORS)
    target_compile_options(cameff_baseline_expert_tests PRIVATE -Werror)
  endif()
endif()

add_test(
  NAME cameff-baseline-expert-tests
  COMMAND cameff_baseline_expert_tests
)

add_executable(cameff_pattern_tests
  tests/test_pattern.c
)

target_link_libraries(cameff_pattern_tests
  PRIVATE cameff_core
)

if(NOT MSVC)
  target_compile_options(cameff_pattern_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
  )

  if(CAMEFF_WARNINGS_AS_ERRORS)
    target_compile_options(cameff_pattern_tests PRIVATE -Werror)
  endif()
endif()

add_test(
  NAME cameff-pattern-tests
  COMMAND cameff_pattern_tests
)
add_executable(cameff_expert_registry_tests
  tests/test_expert_registry.c
)

target_link_libraries(cameff_expert_registry_tests
  PRIVATE cameff_core
)

if(NOT MSVC)
  target_compile_options(cameff_expert_registry_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
  )

  if(CAMEFF_WARNINGS_AS_ERRORS)
    target_compile_options(cameff_expert_registry_tests PRIVATE -Werror)
  endif()
endif()

add_test(
  NAME cameff-expert-registry-tests
  COMMAND cameff_expert_registry_tests
)

add_executable(cameff_specialized_expert_tests
  tests/test_specialized_experts.c
)

target_link_libraries(cameff_specialized_expert_tests
  PRIVATE cameff_core
)

if(NOT MSVC)
  target_compile_options(cameff_specialized_expert_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
  )

  if(CAMEFF_WARNINGS_AS_ERRORS)
    target_compile_options(cameff_specialized_expert_tests PRIVATE -Werror)
  endif()
endif()

add_test(
  NAME cameff-specialized-expert-tests
  COMMAND cameff_specialized_expert_tests
)

add_executable(cameff_fusion_tests
  tests/test_fusion.c
)

target_link_libraries(cameff_fusion_tests
  PRIVATE cameff_core
)

if(NOT MSVC)
  target_compile_options(cameff_fusion_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
  )

  if(CAMEFF_WARNINGS_AS_ERRORS)
    target_compile_options(cameff_fusion_tests PRIVATE -Werror)
  endif()
endif()

add_test(
  NAME cameff-fusion-tests
  COMMAND cameff_fusion_tests
)

add_executable(cameff_evaluation_tests
  tests/test_evaluation.c
)

target_link_libraries(cameff_evaluation_tests
  PRIVATE cameff_core
)

if(NOT MSVC)
  target_compile_options(cameff_evaluation_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
  )

  if(CAMEFF_WARNINGS_AS_ERRORS)
    target_compile_options(cameff_evaluation_tests PRIVATE -Werror)
  endif()
endif()

add_test(
  NAME cameff-evaluation-tests
  COMMAND cameff_evaluation_tests
)

add_executable(cameff_structural_case_tests
  tests/test_structural_cases.c
)

target_link_libraries(cameff_structural_case_tests
  PRIVATE cameff_core
)

if(NOT MSVC)
  target_compile_options(cameff_structural_case_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
  )

  if(CAMEFF_WARNINGS_AS_ERRORS)
    target_compile_options(cameff_structural_case_tests PRIVATE -Werror)
  endif()
endif()

add_test(
  NAME cameff-structural-case-tests
  COMMAND cameff_structural_case_tests
)
install(TARGETS cameff RUNTIME DESTINATION bin)
install(DIRECTORY include/cameff DESTINATION include)

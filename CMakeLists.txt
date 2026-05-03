cmake_minimum_required(VERSION 3.15)
project(astralog-hpc LANGUAGES CXX)

# strict C++17 Standard (Required for std::filesystem and string_view)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Disable compiler-specific extensions (e.g., gnu++17)

# -O3 optimization level required for simdjson
# -march=native: Allows the compiler to use AVX/AVX2/AVX-512 instructions (critical for simdjson)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

#add_executable(astralog 
#    ./src/main.cpp 
#    #./src/Ingestor.cpp
#)

# Integrate simdjson from the external directory
# We compile simdjson.cpp directly into the astralog executable
# target_sources(astralog PRIVATE external/simdjson.cpp)

# Tell the compiler where to look for "simdjson.h" when it sees an #include
# target_include_directories(astralog PRIVATE external)

# --------------------------------------------------------------------
# Catch2 — fetched remotely at configure time via FetchContent
# --------------------------------------------------------------------
include(FetchContent)
 
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1   # pin to a stable release; bump as needed
)
FetchContent_MakeAvailable(Catch2)

# Used to check if Catch's directory is already exist 
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Enable CTest so you can run: cmake --build . && ctest
# ---------------------------------------------------------------------------
enable_testing()
# ---------------------------------------------------------------------------


# Test executable for SimpleRule
add_executable(test_simple_rule
    tests/test_simple_rule_main.cpp
    src/types/rules/SimpleRule.cpp
)
add_executable(test_step_difference_rule
    tests/test_step_difference_rule_main.cpp
    src/types/rules/StepDifferenceRule.cpp
)
add_executable(test_logical_correlation_rule
    tests/test_logical_correlation_rule.cpp
    src/types/rules/LogicalCorrelationRule.cpp
)
target_include_directories(test_simple_rule PRIVATE src)
target_include_directories(test_step_difference_rule PRIVATE src)
target_include_directories(test_logical_correlation_rule PRIVATE src)

# Catch2 TEST
# --------------------------------------------------------------
add_executable(test_simple_rule_main_catch2
    tests/test_simple_rule_main_catch2.cpp
    src/types/rules/SimpleRule.cpp
)
 
target_include_directories(test_simple_rule_main_catch2 PRIVATE src)
 
# Link against Catch2::Catch2WithMain so we don't need to write main() ourselves
target_link_libraries(test_simple_rule_main_catch2 PRIVATE Catch2::Catch2WithMain)
 
# Auto-register every TEST_CASE with CTest
catch_discover_tests(test_simple_rule_main_catch2)

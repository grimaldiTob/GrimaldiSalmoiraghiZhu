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

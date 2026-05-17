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
    GIT_TAG        v3.15.0   # pin to a stable release; bump as needed
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

######################### Test executable for TelemetryBatch #######################
add_executable(test_telemetry_batch
    tests/types/test_TelemetryBatch.cpp
)

# Include directories for test_telemetry_batch
target_include_directories(test_telemetry_batch PRIVATE src)

# Link Catch2 for test_telemetry_batch
target_link_libraries(test_telemetry_batch PRIVATE Catch2::Catch2WithMain)

# Auto-register tests for test_telemetry_batch
catch_discover_tests(test_telemetry_batch)

# Test executable for DataIngestor
add_executable(test_data_ingestor
    tests/components/test_data_ingestor.cpp
    src/components/DataIngestor.cpp
    external/simdjson.cpp
)

# Include directories for test_data_ingestor
target_include_directories(test_data_ingestor PRIVATE src)

# Link Catch2 for test_data_ingestor
target_link_libraries(test_data_ingestor PRIVATE Catch2::Catch2WithMain)

# Auto-register tests for test_data_ingestor
catch_discover_tests(test_data_ingestor)

##################### Test executable for SimpleRule #####################
add_executable(test_simple_rule
    tests/types/test_simple_rule.cpp
    src/types/rules/SimpleRule.cpp
)

# Include directories for test_simple_rule
target_include_directories(test_simple_rule PRIVATE src)

# Link Catch2 for test_simple_rule
target_link_libraries(test_simple_rule PRIVATE Catch2::Catch2WithMain)

# Auto-register tests for test_simple_rule
catch_discover_tests(test_simple_rule)

################# Test executable for StepDifferenceRule ##################
add_executable(test_step_difference_rule
    tests/types/test_step_difference_rule.cpp
    src/types/rules/StepDifferenceRule.cpp
)

# Include directories for test_step_difference_rule
target_include_directories(test_step_difference_rule PRIVATE src)

# Link Catch2 for test_step_difference_rule
target_link_libraries(test_step_difference_rule PRIVATE Catch2::Catch2WithMain)

# Auto-register tests for test_step_difference_rule
catch_discover_tests(test_step_difference_rule)

####################### Test executable for StatefulRule ####################
add_executable(test_stateful_rule
    tests/types/test_stateful_rule.cpp
    src/types/rules/StatefulRule.cpp
)

# Include directories for test_stateful_rule
target_include_directories(test_stateful_rule PRIVATE src)

# Link Catch2 for test_stateful_rule
target_link_libraries(test_stateful_rule PRIVATE Catch2::Catch2WithMain)

# Auto-register tests for test_stateful_rule
catch_discover_tests(test_stateful_rule)

################# Test executable for LogicalCorrelationRule #################
add_executable(test_logical_correlation_rule
    tests/types/test_logical_correlation_rule.cpp
    src/types/rules/LogicalCorrelationRule.cpp
    src/types/rules/SimpleRule.cpp
)

# Include directories for test_logical_correlation_rule
target_include_directories(test_logical_correlation_rule PRIVATE src)

# Link Catch2 for test_logical_correlation_rule
target_link_libraries(test_logical_correlation_rule PRIVATE Catch2::Catch2WithMain)

# Auto-register tests for test_logical_correlation_rule
catch_discover_tests(test_logical_correlation_rule)

##################### Test executable for RuleLoader #########################
add_executable(test_rule_loader
    tests/components/test_RuleLoader.cpp
    src/components/RuleLoader.cpp
    src/types/rules/SimpleRule.cpp
    src/types/rules/StepDifferenceRule.cpp
    src/types/rules/StatefulRule.cpp
    src/types/rules/LogicalCorrelationRule.cpp
    external/simdjson.cpp
)

# Include directories for test_rule_loader
target_include_directories(test_rule_loader PRIVATE src)

# Link Catch2 for test_rule_loader
target_link_libraries(test_rule_loader PRIVATE Catch2::Catch2WithMain)

# Auto-register tests for test_rule_loader
catch_discover_tests(test_rule_loader)

###################### Test executable for ThreadSafeBuffer ####################
add_executable(test_thread_safe_buffer 
    tests/components/test_ThreadSafeBuffer.cpp
)
target_include_directories(test_thread_safe_buffer PRIVATE src)
target_link_libraries(test_thread_safe_buffer PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(test_thread_safe_buffer)

###################### Test executable for BatchAccumulator ####################
add_executable(test_batch_accumulator
    tests/components/test_batch_accumulator.cpp
    src/components/BatchAccumulator.cpp
)
target_include_directories(test_batch_accumulator PRIVATE src)
target_link_libraries(test_batch_accumulator PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(test_batch_accumulator)

###################### Test executable for Integration ####################
add_executable(test_integration
    tests/integrations/test_integration.cpp
    src/components/BatchAccumulator.cpp
    src/components/DataIngestor.cpp
    external/simdjson.cpp
)
target_include_directories(test_integration PRIVATE src)
target_link_libraries(test_integration PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(test_integration)

###################### Test executable for RuleEngine ####################
add_executable(test_rule_engine
    tests/components/test_rule_engine.cpp
    src/components/RuleEngine.cpp
    src/types/rules/SimpleRule.cpp
    src/types/rules/StatefulRule.cpp
    external/simdjson.cpp
)

target_include_directories(test_rule_engine PRIVATE src)
target_link_libraries(test_rule_engine PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(test_rule_engine)
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../../external/simdjson.h"
#include "../../src/components/BatchAccumulator.h"
#include "../../src/components/DataIngestor.h"
#include "../../src/components/ThreadSafeBuffer.h"

TEST_CASE(
    "DataIngestor processes all available text files in the target directory",
    "[DataIngestor][BatchAccumulator]") {
    constexpr size_t BATCH_SIZE = 100;
    constexpr size_t QUEUE_SIZE = 50;

    ThreadSafeBuffer<TelemetryBatch> broker(QUEUE_SIZE);
    BatchAccumulator accumulator(broker, BATCH_SIZE);
    DataIngestor ingestor(accumulator);

    const std::string input_path = "../tests/test_collector_output";
    /**
     * python3 ../src/count_invalid_format_data.py ../collector_output/
     *
     * Files processed: 5
     * Total valid: 240
     * Total invalid: 10
     *
     * Since we have 240 total valid measurements and the batch size is 100,
     * then in the queue there are 2 ready batches and 40 measurements in the
     * batch of the accumulator left
     */
    constexpr size_t EXPECTED_FILE = 5;
    constexpr size_t EXPECTED_TOTAL_VALID = 240;
    constexpr size_t EXPECTED_QUEUE_SIZE = 2;
    constexpr size_t EXPECTED_ACC_BATCH_SIZE = 40;

    size_t fileCount = 0;

    // Quick safety check
    if (!std::filesystem::exists(input_path) ||
        std::filesystem::is_empty(input_path)) {
        std::cout << "Directory is empty or does not exist.\n";
        return;
    }

    // Loop through every single item in the directory
    for (const auto &entry : std::filesystem::directory_iterator(input_path)) {
        // Only process it if it's a .txt file
        if (entry.path().extension() == ".txt") {
            fileCount++;
            std::cout << "Parsing: " << entry.path().filename() << "\n";
            ingestor.parseTelemetry(entry.path().string());
        }
    }

    REQUIRE(fileCount == EXPECTED_FILE);

    // Since we have 240 total valid measurements and the batch size is 100,
    // then in queue there are two ready batches and 40 measurements in the
    // batch of the accumulator

    size_t queueSize = broker.getQueue().size();
    REQUIRE(queueSize == EXPECTED_QUEUE_SIZE);

    size_t accBatchSize = accumulator.getBatchFile().getSize();
    REQUIRE(accBatchSize == EXPECTED_ACC_BATCH_SIZE);

    std::cout << "Finished processing all files." << "\n";
}

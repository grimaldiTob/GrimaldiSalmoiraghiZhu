#include "../../src/components/DataIngestor.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

// Mock class are used mostly when we want a class that imitates
// a specific class whose implementiation is not available yet with
// interface already defined
class MockBatchAccumulator : public BatchAccumulatorInterface {

  public:
    int callCount = 0;
    TelemetryBatch batch;

    void storeValidData(TelemetryBatch &validBatch) override {

        callCount++;
        batch.clear();

        for (size_t i = 0; i < validBatch.getSize(); ++i) {
            batch.emplaceBack(validBatch.sensors_name[i],
                              validBatch.timestamps[i], validBatch.values[i],
                              validBatch.priorities[i]);
        }
    }
};

// RAII helper to safely create and destroy temporary text files during tests
struct TempFile {
    std::string filename;
    TempFile(const std::string &name, const std::string &content)
        : filename(name) {
        std::ofstream out(filename);
        out << content;
        out.close();
    }
    ~TempFile() { std::remove(filename.c_str()); }
};

static int64_t parseISO8601(std::string_view time_str) {
    struct tm tm_struct = {0};
    std::string s(time_str);

    // parse the format YYYY-MM-DDTHH:MM:SSZ
    if (strptime(s.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm_struct) != nullptr) {
        return timegm(&tm_struct); // Convert to Unix epoch
    }
    return 0; // Return 0 if parsing fails
}

TEST_CASE("DataIngestor File Loading and Error Handling",
          "[DataIngestor][FileIO]") {

    MockBatchAccumulator mockAccumulator;
    DataIngestor ingestor(mockAccumulator);

    SECTION("Throws an exception when the file does not exist") {
        std::string badPath = "SimoneRealeTheGOAT.txt";
        REQUIRE_THROWS_AS(ingestor.parseTelemetry(badPath), std::runtime_error);
    }

    SECTION(
        "Throws an exception when the file exists but is completely empty") {
        // Create a temporary file with zero content
        TempFile emptyFile("empty_test_file.txt", "");
        REQUIRE_THROWS_AS(ingestor.parseTelemetry(emptyFile.filename),
                          std::runtime_error);
    }

    SECTION("Successfully loads and reads an existing, populated file") {
        // Create a temporary file with one valid JSON record
        std::string validData = "{\"sensor_id\": \"TEST_01\", \"timestamp\": "
                                "1680000000, \"value\": 42.0}\n";
        TempFile goodFile("good_test_file.txt", validData);

        // We require that the function executes entirely without throwing ANY
        // exceptions
        REQUIRE_NOTHROW(ingestor.parseTelemetry(goodFile.filename));

        // Verify that the file was not only loaded, but the data was actually
        // parsed
        REQUIRE(mockAccumulator.callCount == 1);
    }
}

TEST_CASE("Real File Integration Test", "[Integration][FileIO]") {

    MockBatchAccumulator mockAccumulator;
    DataIngestor ingestor(mockAccumulator);

    // In this particular json file we have a total of 50 measurements, whose
    // 47 have valid format and 3 invalid format
    const auto fixturePath =
        std::filesystem::path(__FILE__).parent_path().parent_path() /
        "test_collector_output" / "raw_data_1777721417_0000.txt";
    // Normalize to an absolute path to prevent simdjson IO_ERRORs.
    std::string absolutePath = std::filesystem::weakly_canonical(
                                   std::filesystem::absolute(fixturePath))
                                   .string();

    // Pre-check: Give a clear Catch2 error if the file is missing from the disk
    std::ifstream checkFile(absolutePath);
    INFO("Could not find the external file at: " << absolutePath);
    REQUIRE(checkFile.good());
    checkFile.close(); // Close it so simdjson can open it

    // Require that the ingestor processes the real file without throwing any
    // exceptions
    REQUIRE_NOTHROW(ingestor.parseTelemetry(absolutePath));

    SECTION("Verify that data is correctly stored") {

        const TelemetryBatch &batch = mockAccumulator.batch;
        const size_t size = mockAccumulator.batch.getSize();

        // Verify that data was actually sent to the accumulator
        REQUIRE(mockAccumulator.callCount == 1);

        // Verify that only valid data are stored
        REQUIRE(size == 47);

        // Check if the FIRST element of the file match with the last element of
        // the batch inside the BatchAccumulator
        CHECK(batch.sensors_name[0] == "TEMP-001");
        CHECK(batch.timestamps[0] == parseISO8601("2026-04-22T16:12:02Z"));
        CHECK(batch.values[0] == Catch::Approx(80.547));
        CHECK(batch.priorities[0] == 1);

        // Check if the LAST element of the file match with the last element of
        // the batch inside the BatchAccumulator
        CHECK(batch.sensors_name[size - 1] == "TEMP-011");
        CHECK(batch.timestamps[size - 1] ==
              parseISO8601("2026-04-22T16:12:09Z"));
        CHECK(batch.values[size - 1] == Catch::Approx(70.299));
        CHECK(batch.priorities[size - 1] == 1);
    }

    // Print a success message to the console showing how many batches it read
    WARN("Success! Read real external file. Batches extracted: "
         << mockAccumulator.callCount);
}

#define CATCH_CONFIG_MAIN
#include "../src/types/TelemetryBatch.h"
#include "../src/types/rules/StatefulRule.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <unordered_map>

// Mock implementation of MeasDatabaseInterface
class MockMeasDatabase : public MeasDatabaseInterface {
  public:
    const std::unordered_map<std::string, std::vector<double>> &
    getMeasHistory() const override {
        return mockMeasurementsHistory;
    }

    void storeResult(std::string sensor_id, double value) override {
        mockMeasurementsHistory[sensor_id].emplace_back(value);
    }

    void clearMeasurements(int n = 32) override {
        for (auto &kv : mockMeasurementsHistory) {
            auto &vec = kv.second;
            if (vec.size() > n) {
                vec.erase(vec.begin(), vec.begin() + n);
            } else {
                vec.clear();
            }
        }
    }

  private:
    std::unordered_map<std::string, std::vector<double>>
        mockMeasurementsHistory;
};

TEST_CASE("StatefulRule evaluates correctly with valid input",
          "[StatefulRule]") {
    // Create a mock database
    MockMeasDatabase mockDb;

    // Create a telemetry batch with some sensor data
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1", "Sensor2"};
    batch.values = {15.0, 20.0};

    // Create a StatefulRule
    StatefulRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 3, 10.0);
    rule.setMeasDatabase(mockDb);

    // Simulate storing measurements in the database
    mockDb.storeResult("Sensor1", 12.0);
    mockDb.storeResult("Sensor1", 14.0);
    mockDb.storeResult("Sensor1", 16.0);

    std::unordered_map<std::string, std::optional<bool>> cache;

    // Testing the evaluate function of StatefulRule
    auto result = rule.evaluate(batch, cache);

    // Assertions
    REQUIRE(result.has_value()); // Check that the result is valid (not nullopt)
    REQUIRE(result.value() == true); // Check that the evaluation result is true
                                     // (12.0, 14.0, 16.0 > 10.0)
}

TEST_CASE("StatefulRule returns nullopt for missing sensor", "[StatefulRule]") {
    // Create a mock database
    MockMeasDatabase mockDb;

    // Create a telemetry batch with some sensor data
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor2"};
    batch.values = {20.0};

    // Create a StatefulRule
    StatefulRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 3, 10.0);
    rule.setMeasDatabase(mockDb);

    std::unordered_map<std::string, std::optional<bool>> cache;

    // Testing the evaluate function of StatefulRule
    auto result = rule.evaluate(batch, cache);

    // Assertions
    REQUIRE(!result.has_value()); // Check that the result is nullopt since
                                  // Sensor1 is missing from the batch
}

TEST_CASE("StatefulRule handles invalid operator", "[StatefulRule]") {
    // Create a mock database
    MockMeasDatabase mockDb;

    // Create a telemetry batch with some sensor data
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1"};
    batch.values = {15.0};

    // Create a StatefulRule with an invalid operator
    StatefulRule rule("Rule1", RulePriority::HIGH, "Sensor1", "INVALID_OP", 3,
                      10.0);
    rule.setMeasDatabase(mockDb);

    std::unordered_map<std::string, std::optional<bool>> cache;

    // Testing the evaluate function of StatefulRule
    auto result = rule.evaluate(batch, cache);

    // Assertions
    REQUIRE(!result.has_value()); // Check that the result is nullopt due to
                                  // invalid operator
}

TEST_CASE("StatefulRule handles invalid DB", "[StatefulRule]") {
    // Create a telemetry batch with some sensor data
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1"};
    batch.values = {15.0};

    // Create a StatefulRule without setting the database
    StatefulRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 3, 10.0);

    std::unordered_map<std::string, std::optional<bool>> cache;

    // Testing the evaluate function of StatefulRule
    auto result = rule.evaluate(batch, cache);

    // Assertions
    REQUIRE(!result.has_value()); // Check that the result is nullopt due to
                                  // invalid database pointer
}

TEST_CASE("StatefulRule returns involved sensors correctly", "[StatefulRule]") {
    // Create a StatefulRule
    StatefulRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 3, 10.0);

    // Assertions
    std::vector<std::string> expected_sensors = {"Sensor1"};
    REQUIRE(rule.getInvolvedSensors() ==
            expected_sensors); // Check that the involved sensors are correct
}

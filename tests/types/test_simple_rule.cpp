#define CATCH_CONFIG_MAIN
#include "../src/types/TelemetryBatch.h"
#include "../src/types/rules/SimpleRule.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <unordered_map>

TEST_CASE("SimpleRule evaluates correctly with valid input", "[SimpleRule]") {
    // Create a telemetry batch with some sensor data
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1", "Sensor2"};
    batch.values = {15.0, 20.0};

    SimpleRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 10.0);
    std::unordered_map<std::string, std::optional<bool>> cache;

    // Testing the evaluate function of SimpleRule
    auto result = rule.evaluate(batch, cache);

    // Assertions
    REQUIRE(result.has_value()); // Check that the result is valid (not nullopt)
    REQUIRE(result.value() ==
            true); // Check that the evaluation result is true (15.0 > 10.0)
}

TEST_CASE("SimpleRule evaluates false with valid input", "[SimpleRule]") {
    // Create a telemetry batch with some sensor data
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1", "Sensor2"};
    batch.values = {15.0, 20.0};

    SimpleRule rule("Rule1", RulePriority::HIGH, "Sensor2", "<", 10.0);
    std::unordered_map<std::string, std::optional<bool>> cache;

    // Testing the evaluate function of SimpleRule
    auto result = rule.evaluate(batch, cache);

    // Assertions
    REQUIRE(result.has_value()); // Check that the result is valid (not nullopt)
    REQUIRE(result.value() == false); // Check that the evaluation result is
                                      // false (5.0 > 10.0 is false)
}

TEST_CASE("SimpleRule returns nullopt for missing sensor", "[SimpleRule]") {
    // Create a telemetry batch with some sensor data
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor2"};
    batch.values = {20.0};

    SimpleRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 10.0);
    std::unordered_map<std::string, std::optional<bool>> cache;

    // Testing the evaluate function of SimpleRule
    auto result = rule.evaluate(batch, cache);

    // Assertions
    REQUIRE(!result.has_value()); // Check that the result is nullopt since
                                  // Sensor1 is missing from the batch
}

TEST_CASE("SimpleRule handles invalid operator", "[SimpleRule]") {
    // Create a telemetry batch with some sensor data
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1"};
    batch.values = {15.0};

    SimpleRule rule("Rule1", RulePriority::HIGH, "Sensor1", "INVALID_OP", 10.0);
    std::unordered_map<std::string, std::optional<bool>> cache;

    // Testing the evaluate function of SimpleRule
    auto result = rule.evaluate(batch, cache);

    // Assertions
    REQUIRE(!result.has_value()); // Check that the result is nullopt since the
                                  // operator is invalid
}

TEST_CASE("SimpleRule returns involved sensors correctly", "[SimpleRule]") {
    SimpleRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 10.0);

    // Testing the getInvolvedSensors function of SimpleRule
    auto involved_sensors = rule.getInvolvedSensors();

    // Assertions
    REQUIRE(involved_sensors.size() ==
            1); // Check that there is exactly one involved sensor
    REQUIRE(involved_sensors[0] ==
            "Sensor1"); // Check that the involved sensor is Sensor1
}

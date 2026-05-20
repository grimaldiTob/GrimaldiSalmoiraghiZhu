#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "../src/types/rules/StepDifferenceRule.h"
#include "../src/types/TelemetryBatch.h"
#include <unordered_map>
#include <optional>

TEST_CASE("StepDifferenceRule evaluates correctly with valid input", "[StepDifferenceRule]") {
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1"};
    batch.values = {10.0};

    StepDifferenceRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 5.0);
    std::unordered_map<std::string, std::optional<bool>> cache;

    // First evaluation should return true as there's no previous value
    auto result = rule.evaluate(batch, cache);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == true);

    // Update the batch with a new value
    batch.values = {20.0};
    result = rule.evaluate(batch, cache);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == true); // Step difference is 10.0 > 5.0
}

TEST_CASE("StepDifferenceRule returns nullopt for missing sensor", "[StepDifferenceRule]") {
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor2"};
    batch.values = {20.0};

    StepDifferenceRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 5.0);
    std::unordered_map<std::string, std::optional<bool>> cache;

    auto result = rule.evaluate(batch, cache);
    REQUIRE(!result.has_value()); // Sensor1 is missing
}

TEST_CASE("StepDifferenceRule handles invalid operator", "[StepDifferenceRule]") {
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1"};
    batch.values = {10.0};

    StepDifferenceRule rule("Rule1", RulePriority::HIGH, "Sensor1", "INVALID_OP", 5.0);
    std::unordered_map<std::string, std::optional<bool>> cache;

    auto result = rule.evaluate(batch, cache);
    REQUIRE(!result.has_value()); // Invalid operator
}

TEST_CASE("StepDifferenceRule returns involved sensors correctly", "[StepDifferenceRule]") {
    StepDifferenceRule rule("Rule1", RulePriority::HIGH, "Sensor1", ">", 5.0);
    std::vector<std::string> expected_sensors = {"Sensor1"};
    REQUIRE(rule.getInvolvedSensors() == expected_sensors); // Check that the involved sensors are correct
}
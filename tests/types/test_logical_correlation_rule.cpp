#define CATCH_CONFIG_MAIN
#include "../src/types/TelemetryBatch.h"
#include "../src/types/rules/LogicalCorrelationRule.h"
#include "../src/types/rules/SimpleRule.h"
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <unordered_map>

TEST_CASE("LogicalCorrelationRule evaluates correctly with AND logic",
          "[LogicalCorrelationRule]") {
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1", "Sensor2"};
    batch.values = {10.0, 20.0};

    auto rule1 = std::make_shared<SimpleRule>("Rule1", RulePriority::HIGH,
                                              "Sensor1", ">", 5.0);
    auto rule2 = std::make_shared<SimpleRule>("Rule2", RulePriority::HIGH,
                                              "Sensor2", "<", 25.0);
    std::vector<std::shared_ptr<BaseRule>> conditions = {rule1, rule2};

    LogicalCorrelationRule corr_rule("CorrRule1", RulePriority::HIGH, "AND",
                                     conditions);
    std::unordered_map<std::string, std::optional<bool>> cache;

    auto result = corr_rule.evaluate(batch, cache);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == true); // Both conditions are true
}

TEST_CASE("LogicalCorrelationRule evaluates correctly with OR logic",
          "[LogicalCorrelationRule]") {
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1", "Sensor2"};
    batch.values = {10.0, 30.0};

    auto rule1 = std::make_shared<SimpleRule>("Rule1", RulePriority::HIGH,
                                              "Sensor1", ">", 5.0);
    auto rule2 = std::make_shared<SimpleRule>("Rule2", RulePriority::HIGH,
                                              "Sensor2", "<", 25.0);
    std::vector<std::shared_ptr<BaseRule>> conditions = {rule1, rule2};

    LogicalCorrelationRule corr_rule("CorrRule2", RulePriority::HIGH, "OR",
                                     conditions);
    std::unordered_map<std::string, std::optional<bool>> cache;

    auto result = corr_rule.evaluate(batch, cache);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == true); // At least one condition is true
}

TEST_CASE(
    "LogicalCorrelationRule works correctly when a rule is already cached",
    "[LogicalCorrelationRule]") {
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1"};
    batch.values = {10.0};

    auto rule1 = std::make_shared<SimpleRule>("Rule1", RulePriority::HIGH,
                                              "Sensor1", ">", 5.0);
    std::vector<std::shared_ptr<BaseRule>> conditions = {rule1};

    LogicalCorrelationRule corr_rule("CorrRule3", RulePriority::HIGH, "AND",
                                     conditions);
    std::unordered_map<std::string, std::optional<bool>> cache;

    // First evaluation to populate the cache
    auto first_result = rule1->evaluate(batch, cache);
    REQUIRE(first_result.has_value());
    REQUIRE(first_result.value() == true);

    // Now evaluate the correlation rule, which should use the cached result for
    // rule1
    auto corr_result = corr_rule.evaluate(batch, cache);
    REQUIRE(corr_result.has_value());
    REQUIRE(corr_result.value() ==
            true); // The correlation rule should also evaluate to true
}

TEST_CASE("LogicalCorrelationRule returns nullopt if a condition rule returns "
          "nullopt",
          "[LogicalCorrelationRule]") {
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1"};
    batch.values = {10.0};

    auto rule1 = std::make_shared<SimpleRule>("Rule1", RulePriority::HIGH,
                                              "Sensor1", ">", 5.0);
    auto rule2 =
        std::make_shared<SimpleRule>("Rule2", RulePriority::HIGH, "Sensor2",
                                     "<", 25.0); // This will return nullopt
    std::vector<std::shared_ptr<BaseRule>> conditions = {rule1, rule2};

    LogicalCorrelationRule corr_rule("CorrRule4", RulePriority::HIGH, "AND",
                                     conditions);
    std::unordered_map<std::string, std::optional<bool>> cache;

    auto result = corr_rule.evaluate(batch, cache);
    REQUIRE(!result.has_value()); // Should return nullopt because rule2 cannot
                                  // be evaluated
}

TEST_CASE("LogicalCorrelationRule stores the result of the conditional rules "
          "in the cache",
          "[LogicalCorrelationRule]") {
    TelemetryBatch batch;
    batch.sensors_name = {"Sensor1", "Sensor2"};
    batch.values = {10.0, 20.0};

    auto rule1 = std::make_shared<SimpleRule>("Rule1", RulePriority::HIGH,
                                              "Sensor1", ">", 5.0);
    auto rule2 = std::make_shared<SimpleRule>("Rule2", RulePriority::HIGH,
                                              "Sensor2", "<", 25.0);
    std::vector<std::shared_ptr<BaseRule>> conditions = {rule1, rule2};

    LogicalCorrelationRule corr_rule("CorrRule5", RulePriority::HIGH, "AND",
                                     conditions);
    std::unordered_map<std::string, std::optional<bool>> cache;

    auto result = corr_rule.evaluate(batch, cache);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == true);
    REQUIRE(cache.find("Rule1") != cache.end());
    REQUIRE(cache.find("Rule2") != cache.end());
    REQUIRE(cache.at("Rule1").has_value());
    REQUIRE(cache.at("Rule2").has_value());
    // I just want to check if the values are there
}

TEST_CASE("LogicalCorrelationRule returns involved sensors correctly",
          "[LogicalCorrelationRule]") {
    auto rule1 = std::make_shared<SimpleRule>("Rule1", RulePriority::HIGH,
                                              "Sensor1", ">", 5.0);
    auto rule2 = std::make_shared<SimpleRule>("Rule2", RulePriority::HIGH,
                                              "Sensor2", "<", 25.0);
    std::vector<std::shared_ptr<BaseRule>> conditions = {rule1, rule2};

    LogicalCorrelationRule corr_rule("CorrRule6", RulePriority::HIGH, "AND",
                                     conditions);

    std::vector<std::string> expected_sensors = {"Sensor1", "Sensor2"};
    REQUIRE(corr_rule.getInvolvedSensors() ==
            expected_sensors); // Check that the involved sensors are correct
}

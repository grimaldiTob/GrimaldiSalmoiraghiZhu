#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include "../src/components/RuleLoader.h"
#include "../external/simdjson.h"
#include "../src/types/rules/SimpleRule.h"
#include "../src/types/rules/StepDifferenceRule.h"
#include "../src/types/rules/StatefulRule.h"
#include "../src/types/rules/LogicalCorrelationRule.h"
#include <memory>
#include <vector>
#include <string>

TEST_CASE("RuleLoader loads rules correctly", "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    simdjson::ondemand::parser parser;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // Create a mock JSON file content
    const std::string json_content = R"([
        {
            "rule_id": "R1",
            "type": "simple",
            "sensor_id": "TEMP-01",
            "operator": ">",
            "value": 50.0,
            "priority": "MEDIUM"
        },
        {
            "rule_id": "R2",
            "type": "step_difference",
            "sensor_id": "PRES-01",
            "operator": "<",
            "value": -2.0,
            "priority": "LOW"
        },
        {
            "rule_id": "R3",
            "type": "stateful",
            "sensor_id": "VOLT-MAIN",
            "operator": "<",
            "value": 20.0,
            "consecutive_measurements": 5,
            "priority": "HIGH"
        },
        {
            "rule_id": "R4",
            "type": "correlation",
            "logic": "AND",
            "conditions": ["R1", "R2"],
            "priority": "HIGH"
        }
    ])";

    // Load the JSON content into a simdjson padded_string
    simdjson::padded_string json_padded(json_content);

    // Convert simdjson::padded_string to std::string
    // Our method loadRules expects a std::string, but
    // simdjson works with padded_string for performance reasons.
    std::string json_string(json_padded.data(), json_padded.size());

    // Act
    ruleLoader.loadRules(parser, json_string, rules_list);

    // Assert
    REQUIRE(rules_list.size() == 4);

    auto simple_rule = std::dynamic_pointer_cast<SimpleRule>(rules_list[0]);
    REQUIRE(simple_rule != nullptr);
    REQUIRE(simple_rule->getRuleId() == "R1");
    REQUIRE(simple_rule->getPriority() == RulePriority::MEDIUM);
    REQUIRE(simple_rule->getSensorId() == "TEMP-01");
    REQUIRE(simple_rule->getOperator() == ">");
    REQUIRE(simple_rule->getValue() == 50.0);

    auto step_diff_rule = std::dynamic_pointer_cast<StepDifferenceRule>(rules_list[1]);
    REQUIRE(step_diff_rule != nullptr);
    REQUIRE(step_diff_rule->getRuleId() == "R2");
    REQUIRE(step_diff_rule->getPriority() == RulePriority::LOW);
    REQUIRE(step_diff_rule->getSensorId() == "PRES-01");
    REQUIRE(step_diff_rule->getOperator() == "<");
    REQUIRE(step_diff_rule->getValue() == -2.0);

    auto stateful_rule = std::dynamic_pointer_cast<StatefulRule>(rules_list[2]);
    REQUIRE(stateful_rule != nullptr);
    REQUIRE(stateful_rule->getRuleId() == "R3");
    REQUIRE(stateful_rule->getPriority() == RulePriority::HIGH);
    REQUIRE(stateful_rule->getSensorId() == "VOLT-MAIN");
    REQUIRE(stateful_rule->getOperator() == "<");
    REQUIRE(stateful_rule->getValue() == 20.0);
    REQUIRE(stateful_rule->getConsecutiveMeas() == 5);

    auto correlation_rule = std::dynamic_pointer_cast<LogicalCorrelationRule>(rules_list[3]);
    REQUIRE(correlation_rule != nullptr);
    REQUIRE(correlation_rule->getRuleId() == "R4");
    REQUIRE(correlation_rule->getPriority() == RulePriority::HIGH);
    REQUIRE(correlation_rule->getLogic() == "AND");
    REQUIRE(correlation_rule->getConditionRuleIds().size() == 2);
    REQUIRE(correlation_rule->getConditionRuleIds()[0]->getRuleId() == "R1");
    REQUIRE(correlation_rule->getConditionRuleIds()[1]->getRuleId() == "R2");
}
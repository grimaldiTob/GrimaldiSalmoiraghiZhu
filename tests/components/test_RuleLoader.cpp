#define CATCH_CONFIG_MAIN
#include "../external/simdjson.h"
#include "../src/components/RuleLoader.h"
#include "../src/types/rules/LogicalCorrelationRule.h"
#include "../src/types/rules/SimpleRule.h"
#include "../src/types/rules/StatefulRule.h"
#include "../src/types/rules/StepDifferenceRule.h"
#include <catch2/catch_test_macros.hpp>
#include <cstdio>  // for std::remove
#include <fstream> // for file operations
#include <memory>
#include <string>
#include <vector>

simdjson::ondemand::parser parser;

TEST_CASE("RuleLoader loads rules correctly", "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // Create a mock JSON file content
    // Rules taken from project PDF description
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
    std::string json_string(json_padded.data(), json_padded.size());

    // Write to a temporary file
    std::string test_file = "test_rules.json";
    std::ofstream file(test_file);
    file << json_content;
    file.close();

    // Act
    ruleLoader.loadRules(test_file, rules_list);

    // Search rules (since they are ordered by priority)
    std::shared_ptr<SimpleRule> simple_rule;
    std::shared_ptr<StepDifferenceRule> step_diff_rule;
    std::shared_ptr<StatefulRule> stateful_rule;
    std::shared_ptr<LogicalCorrelationRule> correlation_rule;

    for (const auto &rule : rules_list) {
        if (rule->getRuleId() == "R1") {
            simple_rule = std::dynamic_pointer_cast<SimpleRule>(rule);
        } else if (rule->getRuleId() == "R2") {
            step_diff_rule =
                std::dynamic_pointer_cast<StepDifferenceRule>(rule);
        } else if (rule->getRuleId() == "R3") {
            stateful_rule = std::dynamic_pointer_cast<StatefulRule>(rule);
        } else if (rule->getRuleId() == "R4") {
            correlation_rule =
                std::dynamic_pointer_cast<LogicalCorrelationRule>(rule);
        }
    }

    // Assert
    SECTION("All rules are loaded") { REQUIRE(rules_list.size() == 4); }

    SECTION("SimpleRule is loaded with correct attributes") {
        REQUIRE(simple_rule != nullptr);
        REQUIRE(simple_rule->getRuleId() == "R1");
        REQUIRE(simple_rule->getPriority() == RulePriority::MEDIUM);
        REQUIRE(simple_rule->getSensorId() == "TEMP-01");
        REQUIRE(simple_rule->getOperator() == ">");
        REQUIRE(simple_rule->getValue() == 50.0);
    }

    SECTION("StepDifferenceRule is loaded with correct attributes") {
        REQUIRE(step_diff_rule != nullptr);
        REQUIRE(step_diff_rule->getRuleId() == "R2");
        REQUIRE(step_diff_rule->getPriority() == RulePriority::LOW);
        REQUIRE(step_diff_rule->getSensorId() == "PRES-01");
        REQUIRE(step_diff_rule->getOperator() == "<");
        REQUIRE(step_diff_rule->getValue() == -2.0);
    }

    SECTION("StatefulRule is loaded with correct attributes") {
        REQUIRE(stateful_rule != nullptr);
        REQUIRE(stateful_rule->getRuleId() == "R3");
        REQUIRE(stateful_rule->getPriority() == RulePriority::HIGH);
        REQUIRE(stateful_rule->getSensorId() == "VOLT-MAIN");
        REQUIRE(stateful_rule->getOperator() == "<");
        REQUIRE(stateful_rule->getValue() == 20.0);
        REQUIRE(stateful_rule->getConsecutiveMeas() == 5);
    }

    SECTION("LogicalCorrelationRule is loaded with correct attributes") {
        REQUIRE(correlation_rule != nullptr);
        REQUIRE(correlation_rule->getRuleId() == "R4");
        REQUIRE(correlation_rule->getPriority() == RulePriority::HIGH);
        REQUIRE(correlation_rule->getLogic() == "AND");
        REQUIRE(correlation_rule->getConditionRuleIds().size() == 2);
        REQUIRE(correlation_rule->getConditionRuleIds()[0]->getRuleId() ==
                "R1");
        REQUIRE(correlation_rule->getConditionRuleIds()[1]->getRuleId() ==
                "R2");
    }

    // Cleanup
    std::remove(test_file.c_str());
}

TEST_CASE("RuleLoader sorts rules by priority", "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    const std::string json_content = R"([
        {
            "rule_id": "R_HIGH",
            "type": "simple",
            "sensor_id": "S1",
            "operator": ">",
            "value": 1.0,
            "priority": "HIGH"
        },
        {
            "rule_id": "R_LOW",
            "type": "simple",
            "sensor_id": "S2",
            "operator": ">",
            "value": 2.0,
            "priority": "LOW"
        },
        {
            "rule_id": "R_MED",
            "type": "simple",
            "sensor_id": "S3",
            "operator": ">",
            "value": 3.0,
            "priority": "MEDIUM"
        }
    ])";

    std::string test_file = "priority_order_rules.json";
    std::ofstream file(test_file);
    file << json_content;
    file.close();

    // Act
    ruleLoader.loadRules(test_file, rules_list);

    // Assert
    REQUIRE(rules_list.size() == 3);
    REQUIRE(rules_list[0]->getPriority() == RulePriority::HIGH);
    REQUIRE(rules_list[0]->getRuleId() == "R_HIGH");
    REQUIRE(rules_list[1]->getPriority() == RulePriority::MEDIUM);
    REQUIRE(rules_list[1]->getRuleId() == "R_MED");
    REQUIRE(rules_list[2]->getPriority() == RulePriority::LOW);
    REQUIRE(rules_list[2]->getRuleId() == "R_LOW");

    // Cleanup
    std::remove(test_file.c_str());
}

TEST_CASE("RuleLoader handles invalid JSON file", "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    simdjson::ondemand::parser parser;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // Act
    ruleLoader.loadRules("non_existent_file.json", rules_list);

    // Assert
    REQUIRE(rules_list.empty());
}

TEST_CASE("RuleLoader handles empty JSON file", "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // Create an empty JSON file
    std::string test_file = "empty_rules.json";
    std::ofstream file(test_file);
    file << "[]"; // Write an empty array to represent no rules
    file.close();

    // Act
    ruleLoader.loadRules(test_file, rules_list);

    // Assert
    REQUIRE(rules_list.empty());

    // Cleanup
    std::remove(test_file.c_str());
}

TEST_CASE("RuleLoader handles invalid JSON content", "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // Create a JSON file with invalid content
    std::string test_file = "invalid_rules.json";
    std::ofstream file(test_file);
    file << "{ invalid json }"; // Write invalid JSON content
    file.close();

    // Act
    ruleLoader.loadRules(test_file, rules_list);

    // Assert
    REQUIRE(rules_list.empty());

    // Cleanup
    std::remove(test_file.c_str());
}

TEST_CASE("RuleLoader handles missing mandatory fields in JSON",
          "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // Create a JSON file with missing fields
    std::string test_file = "missing_fields_rules.json";
    std::ofstream file(test_file);
    file << R"([
        {
            "rule_id": "R1",
            "type": "simple",
            "sensor_id": "TEMP-01",
            "operator": ">",
            "priority": "MEDIUM"
        }
    ])"; // Missing the 'value' field
    file.close();

    // Act
    ruleLoader.loadRules(test_file, rules_list);

    // Assert
    REQUIRE(
        rules_list
            .empty()); // The rule should not be loaded due to missing fields

    // Cleanup
    std::remove(test_file.c_str());
}

TEST_CASE("RuleLoader handles invalid priority value in JSON", "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // Create a JSON file with an invalid priority value
    std::string test_file = "invalid_priority_rules.json";
    std::ofstream file(test_file);
    file << R"([
        {
            "rule_id": "R1",
            "type": "simple",
            "sensor_id": "TEMP-01",
            "operator": ">",
            "value": 50.0,
            "priority": "INVALID_PRIORITY"
        }
    ])"; // Invalid priority value
    file.close();

    // Act
    ruleLoader.loadRules(test_file, rules_list);

    // Assert
    REQUIRE(rules_list.size() ==
            1); // The rule should be loaded with default priority (LOW)
    REQUIRE(rules_list[0]->getPriority() ==
            RulePriority::LOW); // Default to LOW on invalid priority

    // Cleanup
    std::remove(test_file.c_str());
}

TEST_CASE("RuleLoader handles rules with missing optional fields (priority "
          "field) in JSON",
          "[RuleLoader]") {
    // Arrange
    RuleLoader ruleLoader;
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // Create a JSON file with the only missing optional fields (priority)
    // according to RADD
    std::string test_file = "missing_optional_fields_rules.json";
    std::ofstream file(test_file);
    file << R"([
        {
            "rule_id": "R1",
            "type": "simple",
            "sensor_id": "TEMP-01",
            "operator": ">",
            "value": 50.0
        },
        {
            "rule_id": "R2",
            "type": "step_difference",
            "sensor_id": "PRES-01",
            "operator": "<",
            "value": -2.0
        },
        {
            "rule_id": "R3",
            "type": "stateful",
            "sensor_id": "VOLT-MAIN",
            "operator": "<",
            "value": 20.0,
            "consecutive_measurements": 5
        },
        {
            "rule_id": "R4",
            "type": "correlation",
            "logic": "AND",
            "conditions": ["R1", "R2"]
        }
    ])"; // Missing the 'priority' field, should default to LOW
    file.close();

    // Act
    ruleLoader.loadRules(test_file, rules_list);

    // Assert
    REQUIRE(rules_list.size() == 4); // The rule should be loaded
    REQUIRE(rules_list[0]->getPriority() ==
            RulePriority::LOW); // Default to LOW on missing priority
    REQUIRE(rules_list[1]->getPriority() ==
            RulePriority::LOW); // Default to LOW on missing priority
    REQUIRE(rules_list[2]->getPriority() ==
            RulePriority::LOW); // Default to LOW on missing priority
    REQUIRE(rules_list[3]->getPriority() == RulePriority::LOW); // Default

    // Cleanup
    std::remove(test_file.c_str());
}

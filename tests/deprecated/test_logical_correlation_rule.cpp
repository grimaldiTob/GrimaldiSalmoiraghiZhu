#include <iostream>
#include <unordered_map>
# include <optional>

#include "../src/types/rules/LogicalCorrelationRule.hpp"

int main() {
    // First simple example just to test the logic.
    std::cout << "Testing LogicalCorrelationRule..." << std::endl;
    std::cout << "Simple test of evaluate_internal with predefined rule results." << std::endl;
    // Create a logical correlation rule that combines two simple rules with AND logic
    LogicalCorrelationRule and_rule("and_rule", RulePriority::HIGH, "AND", {"rule1", "rule2"});

    // Create a logical correlation rule that combines two simple rules with OR logic
    LogicalCorrelationRule or_rule("or_rule", RulePriority::HIGH, "OR", {"rule1", "rule2"});

    // Simulate the results of the simple rules
    std::unordered_map<std::string, std::optional<bool>> rule_results = {
        {"rule1", true},
        {"rule2", false}
    };

    // Evaluate the AND rule
    std::optional<bool> and_result = and_rule.evaluate_internal(rule_results);
    std::cout << "AND Rule Result: " << (and_result.has_value() ? (and_result.value() ? "True" : "False") : "Not evaluated") << std::endl;

    // Evaluate the OR rule
    std::optional<bool> or_result = or_rule.evaluate_internal(rule_results);
    std::cout << "OR Rule Result: " << (or_result.has_value() ? (or_result.value() ? "True" : "False") : "Not evaluated") << std::endl;

    (( and_result.has_value() && and_result.value() == false) && (or_result.has_value() && or_result.value() == true)) 
        ? std::cout << "Test passed!" << std::endl 
        : std::cout << "Test failed!" << std::endl;
    
    std::cout<<"\n\n" << std::endl; 
    // Test for the evaluate method with a properly formatted input string
    std::cout << "Testing evaluate method with input string 'rule1:true,rule2:false'" << std::endl;
    std::optional<bool> evaluate_result = and_rule.evaluate("rule1:true,rule2:false");
    std::cout << "AND rule result: " << (evaluate_result.has_value() ? (evaluate_result.value() ? "True" : "False") : "Not evaluated") << std::endl;
    std::cout << "OR rule result: " << (or_rule.evaluate("rule1:true,rule2:false").has_value() ? (or_rule.evaluate("rule1:true,rule2:false").value() ? "True" : "False") : "Not evaluated") << std::endl;
    (( and_result.has_value() && and_result.value() == false) && (or_result.has_value() && or_result.value() == true)) 
        ? std::cout << "Test passed!" << std::endl 
        : std::cout << "Test failed!" << std::endl;
    return 0;
}
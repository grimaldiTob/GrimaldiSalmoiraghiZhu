#include "../src/types/rules/StepDifferenceRule.h"
#include <iostream>
#include <optional>

int main() {
    // rule_id, priority, sensor_id, op, value
    StepDifferenceRule rule("rule1", RulePriority::MEDIUM, "temp", "<", 6.0);

    std::string test_inputs[] = {
        "temp,25.5",   // should evaluate true cause its the first measurement
        "temp,15.0",   // should evaluate true 
        "temp,20.0",   // should evaluate false
        "temp,21.0",   // should evaluate false
        "hum,25.5",    // not evaluated (sensor mismatch)
        "temp,abc",    // not evaluated (invalid value)
        "temp25.5",    // not evaluated (invalid format)
        "temp,25.5,extra", // not evaluated (invalid format)
        "temp," // not evaluated (invalid format)
    };
    
    for (const auto& input : test_inputs) {
        std::optional<bool> result = rule.evaluate(input);
        std::cout << "Input: '" << input << "' => ";
        if (!result.has_value()) {
            std::cout << "Not evaluated";
        } else {
            std::cout << (result.value() ? "True" : "False");
        }
        std::cout << std::endl;
    }
    return 0;
}
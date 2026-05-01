#include "SimpleRule.h"

std::optional<bool> SimpleRule::evaluate(const std::string& input) {
    // For now I am assuming input is a string in the format "sensor_id,value"
    // but obviously this depends on how we implement the RuleEngine and how we
    // treat TelemetryBatch inside that component.

    //extract sensor_id and value from input
    int comma_pos = input.find(',');
    // If no , is found, if it's at the end of the string, or if there are multiple commas, 
    // it's an invalid input format -> HOW DO WE ANDLES EXCEPTIONS??
    if (comma_pos == std::string::npos || comma_pos == input.length() - 1 || input.find(',', comma_pos + 1) != std::string::npos) {
        // Invalid input format -> HOW DO WE ANDLES EXCEPTIONS??
        return std::nullopt;
    }

    std::string batch_sensor_id = input.substr(0, comma_pos);
    std::string batch_value_str = input.substr(comma_pos + 1);

    // Convert value to a number (assuming it's a double)
    double batch_value;
    try {
        batch_value = std::stod(batch_value_str);
    } catch (const std::exception&) {
        // Invalid value format -> HOW DO WE ANDLES EXCEPTIONS??
        return std::nullopt;
    }

    // Check if the sensor_id matches
    if (this -> sensor_id != batch_sensor_id) {
        return std::nullopt;
    }

    // Evaluate the rule based on the operator 
    // I don't like this verbose if-else structure, but I'll leave it for now. 
    // I am sure we could replace it with some kind of map from operator to lambda...
    if (op == "==") return batch_value == value;
    else if (op == "!=") return batch_value != value;
    else if (op == "<") return batch_value < value;
    else if (op == "<=") return batch_value <= value;
    else if (op == ">") return batch_value > value;
    else if (op == ">=") return batch_value >= value;
    else {
        // Invalid operator -> HOW DO WE ANDLES EXCEPTIONS??
        return std::nullopt;
    }
    return std::nullopt;
}
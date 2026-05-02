#include "StepDifferenceRule.h"

std::optional<bool> StepDifferenceRule::evaluate(const std::string& input) {
    // extract sensor_id and value from input
    size_t comma_pos = input.find(',');
    
    if (comma_pos == std::string::npos || comma_pos == input.length() - 1 || input.find(',', comma_pos + 1) != std::string::npos) {
        return std::nullopt; // Invalid input format
    }

    std::string batch_sensor_id = input.substr(0, comma_pos);
    std::string batch_value_str = input.substr(comma_pos + 1);

    double current_value;
    try {
        current_value = std::stod(batch_value_str);
    } catch (const std::exception&) {
        return std::nullopt; // Invalid number format
    }

    // case in which there is no previous value
    if (!previous_value.has_value()) {
        // yhis is the very first reading so we cannot calculate a difference yet.
        previous_value = current_value;
        return true; // I guess we just return true ??? 
    }
    /*
        Here we do not need a routine that accesses the "Db" structure.
        Instead of searching in the db each time. We can store the value in a 
        variable and update each time it gets evaluated. 
    */
    
    double step_diff = std::abs(current_value - previous_value.value());
    previous_value = current_value; // update the previous value

    if (op == "==") return step_diff == value;
    else if (op == "!=") return step_diff != value;
    else if (op == "<") return step_diff < value;
    else if (op == "<=") return step_diff <= value;
    else if (op == ">") return step_diff > value;
    else if (op == ">=") return step_diff >= value;
    
    return std::nullopt; // Invalid operator
}
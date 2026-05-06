#include "SimpleRule.h"

std::optional<bool> SimpleRule::evaluate(BatchAccumulator& accumulator, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {
    if(cache.count(this->rule_id)){
        return cache[this->rule_id]; // return the result stored in the cache
    }
    bool rule = false;
    bool sensor_found = false;

    // to implement the batch size function
    for (size_t i = 0; i < accumulator.getBatchSize(); ++i){
        if(accumulator.getBatchFile().sensors_name[i] == this->sensor_id){
            sensor_found = true;
            double current_value = accumulator.getBatchFile().values[i];

            if (op == "==")
                return current_value == value;
            else if (op == "!=")
                return current_value != value;
            else if (op == "<")
                return current_value < value;
            else if (op == "<=")
                return current_value <= value;
            else if (op == ">")
                return current_value > value;
            else if (op == ">=")
                return current_value >= value;
            else
            {
                // Invalid operator -> HOW DO WE ANDLES EXCEPTIONS??
                return std::nullopt;
            }
        }
    }
    if (!sensor_found) {
        return std::nullopt; // Sensor wasn't in this batch at all
    }

    return rule;
}
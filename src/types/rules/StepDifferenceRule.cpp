#include "StepDifferenceRule.h"

std::optional<bool> StepDifferenceRule::evaluate(TelemetryBatch& batch, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {

    if(cache.count(this->rule_id)){
        return cache[this->rule_id]; // return the result stored in the cache
    }
    bool sensor_found = false;

    for (size_t i = 0; i < batch.getSize(); ++i){
        if(batch.sensors_name[i] == this->sensor_id){
            sensor_found = true;
            double current_value = batch.values[i];

            // case in which there is no previous value
            if (!previous_value.has_value()) {
                // This is the very first reading so we cannot calculate a difference yet.
                previous_value = current_value;
                return true; // I guess we just return true ??? 
            }
            double step_diff = std::abs(current_value - previous_value.value());
            previous_value = current_value; // update the previous value

            if (op == "==") return step_diff == value;
            else if (op == "!=") return step_diff != value;
            else if (op == "<") return step_diff < value;
            else if (op == "<=") return step_diff <= value;
            else if (op == ">") return step_diff > value;
            else if (op == ">=") return step_diff >= value;            
        }
    }
    if (!sensor_found) {
        return std::nullopt; // Sensor wasn't in this batch at all
    }

    return false; // Default rule evaluation result
}
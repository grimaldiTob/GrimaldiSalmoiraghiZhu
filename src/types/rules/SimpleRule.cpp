#include "SimpleRule.h"

std::optional<bool> SimpleRule::evaluate(TelemetryBatch& batch, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {
    if(cache.count(this->rule_id)){
        return cache[this->rule_id]; // return the result stored in the cache
    }
    bool sensor_found = false;

    for (size_t i = 0; i < batch.getSize(); ++i){
        if(batch.sensors_name[i] == this->sensor_id){
            sensor_found = true;
            double current_value = batch.values[i];

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
                // Invalid operator
                return std::nullopt;
            }
        }
    }
    if (!sensor_found) {
        return std::nullopt; // Sensor wasn't in this batch at all
    }

    return false; // Default rule evaluation result
}
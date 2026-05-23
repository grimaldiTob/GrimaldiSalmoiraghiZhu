#include "StepDifferenceRule.h"

std::optional<bool> StepDifferenceRule::evaluate(const TelemetryBatch& batch, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {
    
    // Check if the database pointer is valid
    if (!database) {
        return std::nullopt;
    }

    if(cache.count(this->rule_id)){
        return cache[this->rule_id]; // return the result stored in the cache
    }

    // Validate operator before processing
    if (op != "==" && op != "!=" && op != "<" && op != "<=" && op != ">" && op != ">=") {
        return std::nullopt; // Invalid operator
    }

    bool sensor_found = false;

    for (size_t i = 0; i < batch.getSize(); ++i){
        if(batch.sensors_name[i] == this->sensor_id){
            sensor_found = true;
            double current_value = batch.values[i];

            // Access the measurement history from the database
            const auto& history = database->getMeasHistory();

            if(history.find(sensor_id) == history.end()) {
                return true; // no previous value loaded in the meas history yet
            }
            // retrieve the previous value
            const double previous_value = history.at(sensor_id).back();

            double step_diff = std::abs(current_value - previous_value);

            if (op == "==") return step_diff == value;
            else if (op == "!=") return step_diff != value;
            else if (op == "<") return step_diff < value;
            else if (op == "<=") return step_diff <= value;
            else if (op == ">") return step_diff > value;
            else if (op == ">=") return step_diff >= value;
        }
    }
    /*
    no point having this here if we return std::nullopt anyways.
    if (!sensor_found) {
        return std::nullopt; // Sensor wasn't in this batch at all
    }
    */ 

    return std::nullopt; // Default to nullopt for any other case
}
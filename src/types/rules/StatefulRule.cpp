#include "StatefulRule.h"
#include <algorithm> // for std::find

std::optional<bool> StatefulRule::evaluate(
    const TelemetryBatch &batch,
    std::unordered_map<std::string, std::optional<bool>> &cache) {

    // Check if the database pointer is valid
    if (!database) {
        return std::nullopt;
    }

    // Check if the operator is valid
    if (oprtor != "==" && oprtor != "!=" && oprtor != "<" && oprtor != "<=" &&
        oprtor != ">" && oprtor != ">=") {
        return std::nullopt; // Invalid operator
    }

    // Check if the result is already cached
    if (cache.count(this->rule_id)) {
        return cache[this->rule_id];
    }

    std::optional<bool> rule = std::nullopt;
    bool sensor_found = false;

    for (size_t i = 0; i < batch.getSize(); ++i) {
        if (batch.sensors_name[i] == this->sensor_id) {
            sensor_found = true;

            // Access the measurement history from the database
            const auto &history = database->getMeasHistory();
            if (history.find(sensor_id) == history.end() ||
                history.at(sensor_id).size() < consecutive_meas - 1) {
                return true; // Not enough history, return true
            }

            const std::vector<double> &hist = history.at(sensor_id);
            std::vector<double> past_meas(
                hist.end() - consecutive_meas + 1,
                hist.end()); // Last `consecutive_meas` elements
            double current_value = batch.values[i];

            // if the rule is true for the current measurement return true
            if (oprtor == "==") {
                if (current_value == value)
                    rule = true;
            } else if (oprtor == "!=") {
                if (current_value != value)
                    rule = true;
            } else if (oprtor == "<") {
                if (current_value < value)
                    rule = true;
            } else if (oprtor == "<=") {
                if (current_value <= value)
                    rule = true;
            } else if (oprtor == ">") {
                if (current_value > value)
                    rule = true;
            } else if (oprtor == ">=") {
                if (current_value >= value)
                    rule = true;
            }

            if (rule)
                break;

            // check all past measurements
            for (double past_value : past_meas) {
                // I dont want to return false because I need to check all the
                // values
                if (oprtor == "==" && past_value == value) {
                    rule = true;
                } else if (oprtor == "!=" && past_value != value) {
                    rule = true;
                } else if (oprtor == "<" && past_value < value) {
                    rule = true;
                } else if (oprtor == "<=" && past_value <= value) {
                    rule = true;
                } else if (oprtor == ">" && past_value > value) {
                    rule = true;
                } else if (oprtor == ">=" && past_value >= value) {
                    rule = true;
                }
                if (rule)
                    break;
            }
        }
    }

    if (!sensor_found) {
        return std::nullopt; // Sensor wasn't in this batch at all
    }

    return rule;
}

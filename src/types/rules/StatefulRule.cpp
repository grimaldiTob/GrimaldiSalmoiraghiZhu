#include "StatefulRule.h"

std::optional<bool> StatefulRule::evaluate(TelemetryBatch& batch, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {

    if (!database) {
        return std::nullopt;
    }

    if(cache.count(this->rule_id)){
        return cache[this->rule_id];
    }

    bool rule = false;
    bool sensor_found = false;

    for(size_t i = 0; i < batch.getSize(); ++i) {
        if(batch.sensors_name[i] == this->sensor_id) {
            sensor_found = true;

            // Access the measurement history from the database
            const auto& history = database->getMeasHistory();
            if (history.find(sensor_id) == history.end() || history.at(sensor_id).size() < consecutive_meas - 1) {
                return true; // Not enough history, return true? What if there are enough measurements, but they are not 
                // from consecutive time steps? 
                // Say we need five measurements, but the history contains t_{n - 1}, t_{n - 3}, t_{n - 4}, t_{n - 5}, t_{n - 6} (missing t_{n - 2}), 
                //should we return true or false? 
                // In fact, I am not even sure are present database allows us to retrieve the measurements 
                //in a way that we can check if they are from consecutive time steps or not, 
                //since we are just storing the last n measurements without timestamps.

                // Ok approaches are two in this case. Either we store a vector of tuples...(value, timestamp)
                // or any possible way of measuring consecutive measuremets, or we put a NaN in the moment in which
                // there is no measurement. 
                // Also, what to do if there is a missing measurement? Consider the rule as true, or false and continue checking?

            }

            const std::vector<double>& hist = history.at(sensor_id);
            std::vector<double> past_meas(hist.end() - consecutive_meas + 1, hist.end()); // Last `consecutive_meas` elements
            double current_value = batch.values[i];

            /* Ok I'm not a fan of lambdas but in this case MAYBE we could be getting 
            better code since this looks like a mess. */

            // if the rule is true for the current measurement return true
            if (oprtor == "==") {
                if (current_value == value) rule = true;
            } 
            else if (oprtor == "!=") {
                if (current_value != value) rule = true;
            } 
            else if (oprtor == "<") {
                if (current_value < value) rule = true;
            } 
            else if (oprtor == "<=") {
                if (current_value <= value) rule = true;
            } 
            else if (oprtor == ">") {
                if (current_value > value) rule = true;
            } 
            else if (oprtor == ">=") {
                if (current_value >= value) rule = true;
            } 
            else {
                return std::nullopt; // return error if the operator is wrong           
            }        

            if(rule) break;

            // check all past measurements
            for (double past_value : past_meas) {
                // I dont want to return false because I need to check all the values
                if (oprtor == "==" && past_value == value) {
                    rule = true;
                } 
                else if (oprtor == "!=" && past_value != value) {
                    rule = true;
                } 
                else if (oprtor == "<" && past_value < value) {
                    rule = true;
                } 
                else if (oprtor == "<=" && past_value <= value) {
                    rule = true;
                } 
                else if (oprtor == ">" && past_value > value) {
                    rule = true;
                } 
                else if (oprtor == ">=" && past_value >= value) {
                    rule = true;
                } 
                else {
                    return std::nullopt; 
                }
                if(rule) break;
            }
        }
    }

    cache[this->rule_id] = rule;
    return rule;
}
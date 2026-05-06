#include "StatefulRule.h"

std::optional<bool> StatefulRule::evaluate(BatchAccumulator& accumulator, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {

    if(cache.count(this->rule_id)){
        return cache[this->rule_id];
    }
    bool rule = false;
    bool sensor_found = false;

    for (size_t i = 0; i < accumulator.getBatchSize();++i) {
        if(accumulator.getBatchFile().sensors_name[i] == this->sensor_id) {
            sensor_found = true;

            const std::vector<double>& hist = accumulator.getMeasurementsHistory().at(sensor_id);
            if(hist.empty() || hist.size() < consecutive_meas - 1)
                return true; // return true if there are no past measurement or there is not at least n measurements in the history array
            std::vector<double> past_meas(hist.end() - consecutive_meas + 1, hist.end()); // take the last `consecutive_meas` element
            double current_value = accumulator.getBatchFile().values[i];

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
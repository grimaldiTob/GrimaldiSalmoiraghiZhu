#include "LogicalCorrelationRule.h"

#include <algorithm>
#include <sstream>

std::optional<bool> LogicalCorrelationRule::evaluate(
    const TelemetryBatch &batch,
    std::unordered_map<std::string, std::optional<bool>> &cache) {
    if (cache.count(this->rule_id)) {
        return cache[this->rule_id];
    }

    std::optional<bool> final_result = std::nullopt;

    if (logic == "AND") {
        for (auto &child : condition_rules) {
            // if the rule is in the cache continues
            if (cache.count(child->getRuleId())) {
                if (!cache.at(child->getRuleId())) {
                    final_result = false;
                    break;
                } else
                    continue;
            }
            // Call the evaluate function for the child rule
            std::optional<bool> child_result = child->evaluate(batch, cache);
            cache[child->getRuleId()] = child_result;
            try {
                if (!child_result.value()) {
                    final_result = false;
                    break; // We just need one rule to be false in this case
                }
            } catch (const std::exception &e) {
                // One of the child rules returned nullopt, we can consider this
                // as a failure for the AND logic
                return std::nullopt;
            }
        }
        final_result = true; // Set to true if no rule is false
    } else if (logic == "OR") {
        for (const auto &child : condition_rules) {
            // reverse logic here --> if the rule is false continue and check
            // the next
            if (cache.count(child->getRuleId())) {
                if (cache.at(child->getRuleId())) {
                    final_result = true;
                    break;
                } else
                    continue;
            }
            std::optional<bool> child_result = child->evaluate(batch, cache);
            cache[child->getRuleId()] =
                child_result; // remember to store the result of these rules

            try {
                if (child_result.value()) {
                    final_result = true;
                    break; // It is enough if one rule is true
                }
            } catch (const std::exception &e) {
                // One of the child rules returned nullopt, we can consider this
                // as a failure for the OR logic
                return std::nullopt;
            }
        }
    }
    return final_result;
}

std::vector<std::string> LogicalCorrelationRule::getInvolvedSensors() const {
    std::vector<std::string> sensors;
    for (const auto &child : condition_rules) {
        const auto &child_sensors = child->getInvolvedSensors();
        sensors.insert(sensors.end(), child_sensors.begin(),
                       child_sensors.end());
    }
    // Remove duplicates
    std::sort(sensors.begin(), sensors.end());
    sensors.erase(std::unique(sensors.begin(), sensors.end()), sensors.end());
    return sensors;
}

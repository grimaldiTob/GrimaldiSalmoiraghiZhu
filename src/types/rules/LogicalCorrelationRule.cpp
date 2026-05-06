#include "LogicalCorrelationRule.h"

#include <sstream>
#include <algorithm>

/*
std::optional<bool> LogicalCorrelationRule::evaluate_internal(
	const std::unordered_map<std::string, std::optional<bool>>& rule_results
) const {
	if (logic == "AND") {
		for (const auto& id : condition_rules) {
            // If any of the rules is false or nullopt, the result is false
			if (!rule_results.at(id).has_value() || !rule_results.at(id).value())
				return false;
		}
		return true;
	} else if (logic == "OR") {
		for (const auto& id : condition_rules) {
            // If any of the rules is true, the result is true
			if (rule_results.at(id).has_value() && rule_results.at(id).value())
				return true;
		}
		return false;
	}
	return std::nullopt; // Invalid logic operator
}
*/

std::optional<bool> LogicalCorrelationRule::evaluate(BatchAccumulator& accumulator, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {
	if (cache.count(this->rule_id)) {
        return cache[this->rule_id];
    }

    bool final_result = false;

    if (logic == "AND") {
		for (auto& child : condition_rules) {
			
			// call the evaluate function for the SimpleRule
            std::optional<bool> child_result = child->evaluate(accumulator, cache); 
            
            if (!child_result.value()) {
				final_result = false;
                break; // we just need one rule to be false in this case
            }
        }
		final_result = true; // Set to true if no rule is false
    } 
    else if (logic == "OR") {
		for (const auto& child : condition_rules) {
			auto child_result = child->evaluate(accumulator, cache);
            if (child_result.value()) {
				final_result = true;
                break; // it is enough one rule is true
            }
        }
    }

    cache[this->rule_id] = final_result;
    return final_result;

}


/*
std::optional<bool> LogicalCorrelationRule::evaluate(const BatchAccumulator& accumulator, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {
	if (input.empty()) return std::nullopt;

	std::unordered_map<std::string, std::optional<bool>> rule_results;
	std::istringstream ss(input);
	std::string token;

	while (std::getline(ss, token, ',')) {
		size_t colon = token.find(':');
		if (colon == std::string::npos) continue; // skip malformed
		std::string rule_id = token.substr(0, colon);
		std::string value = token.substr(colon + 1);
		// Only process expected rule IDs
		if (std::find(condition_rules.begin(), condition_rules.end(), rule_id) == condition_rules.end())
			continue;
		if (value == "true") rule_results[rule_id] = true;
		else if (value == "false") rule_results[rule_id] = false;
		else rule_results[rule_id] = std::nullopt;
	}
	// Check that all required rule IDs are present
	for (const auto& id : condition_rules) {
		if (rule_results.find(id) == rule_results.end())
			return std::nullopt;
	}
	return evaluate_internal(rule_results);
}
*/

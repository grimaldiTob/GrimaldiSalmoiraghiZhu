#include "LogicalCorrelationRule.hpp"

#include <sstream>
#include <algorithm>

std::optional<bool> LogicalCorrelationRule::evaluate_internal(
	const std::unordered_map<std::string, std::optional<bool>>& rule_results
) const {
	if (logic == "AND") {
		for (const auto& id : condition_rule_ids) {
            // If any of the rules is false or nullopt, the result is false
			if (!rule_results.at(id).has_value() || !rule_results.at(id).value())
				return false;
		}
		return true;
	} else if (logic == "OR") {
		for (const auto& id : condition_rule_ids) {
            // If any of the rules is true, the result is true
			if (rule_results.at(id).has_value() && rule_results.at(id).value())
				return true;
		}
		return false;
	}
	return std::nullopt; // Invalid logic operator
}

// Example input format: "R1:true,R2:false,R3:null"
std::optional<bool> LogicalCorrelationRule::evaluate(const std::string& input) {
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
		if (std::find(condition_rule_ids.begin(), condition_rule_ids.end(), rule_id) == condition_rule_ids.end())
			continue;
		if (value == "true") rule_results[rule_id] = true;
		else if (value == "false") rule_results[rule_id] = false;
		else rule_results[rule_id] = std::nullopt;
	}
	// Check that all required rule IDs are present
	for (const auto& id : condition_rule_ids) {
		if (rule_results.find(id) == rule_results.end())
			return std::nullopt;
	}
	return evaluate_internal(rule_results);
}

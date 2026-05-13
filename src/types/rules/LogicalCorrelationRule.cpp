#include "LogicalCorrelationRule.h"

#include <sstream>
#include <algorithm>

std::optional<bool> LogicalCorrelationRule::evaluate(const TelemetryBatch& batch, 
        std::unordered_map<std::string, std::optional<bool>>& cache) {
	if (cache.count(this->rule_id)) {
        return cache[this->rule_id];
    }

    bool final_result = false;

    if (logic == "AND") {
		for (auto& child : condition_rules) {
			// Call the evaluate function for the child rule
            std::optional<bool> child_result = child->evaluate(batch, cache); 
            
            if (!child_result.value()) {
				final_result = false;
                break; // We just need one rule to be false in this case
            }
        }
		final_result = true; // Set to true if no rule is false
    } 
    else if (logic == "OR") {
		for (const auto& child : condition_rules) {
			auto child_result = child->evaluate(batch, cache);
            if (child_result.value()) {
				final_result = true;
                break; // It is enough if one rule is true
            }
        }
    }

    cache[this->rule_id] = final_result;
    return final_result;

}

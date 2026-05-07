#include "LogicalCorrelationRule.h"

#include <sstream>
#include <algorithm>

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

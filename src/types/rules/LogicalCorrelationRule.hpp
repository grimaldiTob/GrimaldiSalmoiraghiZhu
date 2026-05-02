#ifndef LOGICALCORRELATIONRULE_H
#define LOGICALCORRELATIONRULE_H

#include "BaseRule.h"
#include <vector>
#include <unordered_map>

class LogicalCorrelationRule : public BaseRule {
public:
    LogicalCorrelationRule(
        const std::string& rule_id,
        RulePriority priority,
        const std::string& logic, // either "AND" or "OR"
        const std::vector<std::string>& condition_rule_ids
    ) : BaseRule(rule_id, RuleType::CORRELATION, priority),
        logic(logic),
        condition_rule_ids(condition_rule_ids) {}


    // What am I about to do is an incredible mess, I know. 
    // I am creating a evaluate_internal method that takes a map of rule results, 
    // so that I can use it both in the evaluate method (which is required by the BaseRule interface) 
    // and in a more flexible way for direct use.
    // The proper evaluate method parses an input string to create a map
    // (a highly stupid idea), but I needed it to satisfy the interface.
    // We should proprably refactor the BaseClass using some of those strange thigs Formaggia 
    // was supposed to teach us? Possibly. How can we do that? No idea.

    // Flexible method for direct use
    std::optional<bool> evaluate_internal(
        const std::unordered_map<std::string, std::optional<bool>>& rule_results
    ) const;

    // Override required by BaseRule
    std::optional<bool> evaluate(const std::string& input) override;

    std::string getLogic() const { return logic; }
    const std::vector<std::string>& getConditionRuleIds() const { return condition_rule_ids; }

private:
    std::string logic; // "AND" or "OR"
    std::vector<std::string> condition_rule_ids;   // IDs of rules to combine
};

#endif // LOGICALCORRELATIONRULE_H
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
        const std::vector<std::shared_ptr<BaseRule>>& condition_rules_
    ) : BaseRule(rule_id, RuleType::CORRELATION, priority),
        logic(logic),
        condition_rules(condition_rules_) {}


    // Override required by BaseRule
    std::optional<bool> evaluate(BatchAccumulator& accumulator, 
        std::unordered_map<std::string, std::optional<bool>>& cache) override;

    std::string getLogic() const { return logic; }
    const std::vector<std::shared_ptr<BaseRule>>& getConditionRuleIds() const { return condition_rules; }

private:
    std::string logic; // "AND" or "OR"
    std::vector<std::shared_ptr<BaseRule>> condition_rules;   // IDs of rules to combine
};

#endif // LOGICALCORRELATIONRULE_H
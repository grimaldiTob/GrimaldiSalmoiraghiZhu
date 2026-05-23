#pragma once

#include "BaseRule.h"
#include <memory>
#include <unordered_map>
#include <vector>

class LogicalCorrelationRule : public BaseRule {
  public:
    /**
     * @brief Constructor for LogicalCorrelationRule.
     * @param rule_id Unique identifier for the rule.
     * @param priority Priority level of the rule (HIGH, MEDIUM, LOW).
     * @param logic Logical operator for combining conditions ("AND" or "OR").
     * @param condition_rules_ Vector of shared pointers to the rules to
     * combine.
     */
    LogicalCorrelationRule(
        const std::string &rule_id, RulePriority priority,
        const std::string &logic, // either "AND" or "OR"
        const std::vector<std::shared_ptr<BaseRule>> &condition_rules_)
        : BaseRule(rule_id, RuleType::CORRELATION, priority), logic(logic),
          condition_rules(condition_rules_) {}

    // Override required by BaseRule
    std::optional<bool> evaluate(
        const TelemetryBatch &batch,
        std::unordered_map<std::string, std::optional<bool>> &cache) override;

    std::string getLogic() const { return logic; }
    const std::vector<std::shared_ptr<BaseRule>> &getConditionRuleIds() const {
        return condition_rules;
    }

    std::vector<std::string> getInvolvedSensors()
        const override; // List all involved sensor from the condition rules

  private:
    std::string logic; // "AND" or "OR"
    std::vector<std::shared_ptr<BaseRule>>
        condition_rules; // IDs of rules to combine
};

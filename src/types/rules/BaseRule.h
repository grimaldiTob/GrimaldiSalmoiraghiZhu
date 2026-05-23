#pragma once

#include "../TelemetryBatch.h"
#include <cmath>
#include <optional>
#include <string>
#include <unordered_map>

enum class RuleType { SIMPLE, STEP_DIFFERENCE, STATEFUL, CORRELATION };

enum class RulePriority { LOW, MEDIUM, HIGH };

class BaseRule {
  public:
    BaseRule(const std::string &rule_id, const RuleType type,
             const RulePriority priority)
        : rule_id(rule_id), type(type), priority(priority) {}
    virtual ~BaseRule() = default;
    virtual std::optional<bool>
    evaluate(const TelemetryBatch &batch,
             std::unordered_map<std::string, std::optional<bool>> &cache) = 0;
    // I changed the return type to std::optional<bool> to allow for a "null"
    // state in case of invalid input or other issues during evaluation.

    std::string getRuleId() const { return rule_id; }
    RuleType getType() const { return type; }
    RulePriority getPriority() const { return priority; }
    virtual std::vector<std::string> getInvolvedSensors() const {
        return involved_sensors;
    }

  protected:
    const std::string rule_id;
    const RuleType type;
    const RulePriority priority;
    const std::vector<std::string>
        involved_sensors; // Tracks all involved rules, useful for correlation
                          // rules, used by OutputDispatcher
};

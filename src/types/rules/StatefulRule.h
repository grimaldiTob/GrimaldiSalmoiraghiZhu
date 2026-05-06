#ifndef STATEFULRULE_H
#define STATEFULRULE_H

#include "BaseRule.h"
#include <vector>
#include <unordered_map>

class StatefulRule : public BaseRule {
public:
    StatefulRule(
        const std::string& rule_id,
        RulePriority priority,
        const std::string& sensor_id,
        const std::string& oprtor, 
        const double consecutive_meas,
        const double value
    ) : BaseRule(rule_id, RuleType::STATEFUL, priority),
        sensor_id(sensor_id),
        oprtor(oprtor),
        consecutive_meas(consecutive_meas),
        value(value) {}

    // Override required by BaseRule
    std::optional<bool> evaluate(const BatchAccumulator& accumulator, 
        std::unordered_map<std::string, std::optional<bool>>& cache) override;

    std::string getSensorId() const { return sensor_id; }
    std::string getOperator() const { return oprtor; }
    double getValue() const { return value; }
    double getConsecutiveMeas() const { return consecutive_meas; }

private:
    const std::string sensor_id;
    const std::string oprtor;
    const double value;
    const double consecutive_meas;
};

#endif // LOGICALCORRELATIONRULE_H
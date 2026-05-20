#pragma once

#include "BaseRule.h"

class SimpleRule : public BaseRule {
public:
    /**
     * @brief Constructor for SimpleRule.
     * @param rule_id Unique identifier for the rule.
     * @param priority Priority level of the rule (HIGH, MEDIUM, LOW).
     * @param sensor_id Identifier of the sensor to which the rule applies.
     * @param op Operator for comparison (e.g., ">", "<", "==").
     * @param value Value to compare the sensor reading against.
     */
    SimpleRule(const std::string& rule_id, 
               RulePriority priority, 
               const std::string& sensor_id,
               const std::string& op,
               const double value) // no need of passing value by reference
        : BaseRule(rule_id, RuleType::SIMPLE, priority), 
          sensor_id(sensor_id), 
          op(op), 
          value(value) {}

    // Overridden evaluate method (implementation in .cpp file)
    std::optional<bool> evaluate(const TelemetryBatch& batch, 
        std::unordered_map<std::string, std::optional<bool>>& cache) override;

    std::string getSensorId() const { return sensor_id; }
    std::string getOperator() const { return op; }
    double getValue() const { return value; }
    std::vector<std::string> getInvolvedSensors() const { return {sensor_id}; } // {} on the fly vector constructor 

    // Destructor is just defaulted, as there are no resources to manage
    ~SimpleRule() override = default;
private:
    const std::string sensor_id;
    const std::string op;
    const double value;
};


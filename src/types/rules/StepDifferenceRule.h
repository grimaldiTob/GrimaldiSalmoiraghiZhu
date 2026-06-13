#pragma once

#include "../../interfaces/MeasDatabaseInterface.h"
#include "BaseRule.h"

class StepDifferenceRule : public BaseRule {
  public:
    /**
     * @brief Constructor for StepDifferenceRule.
     * @param rule_id Unique identifier for the rule.
     * @param priority Priority level of the rule (HIGH, MEDIUM, LOW).
     * @param sensor_id Identifier of the sensor to which the rule applies.
     * @param op Operator for comparison (e.g., ">", "<", "==").
     * @param value Value to compare the sensor reading against.
     * @param previous_value optional parameter to store the previous sensor
     * value for step difference calculation.
     */
    StepDifferenceRule(const std::string &rule_id, RulePriority priority,
                       const std::string &sensor_id, const std::string &op,
                       const double &value)
        : BaseRule(rule_id, RuleType::STEP_DIFFERENCE, priority),
          sensor_id(sensor_id), op(op), value(value) {}

    std::optional<bool> evaluate(
        const TelemetryBatch &batch,
        std::unordered_map<std::string, std::optional<bool>> &cache) override;

    void setMeasDatabase(MeasDatabaseInterface &db) { database = &db; }

    std::string getSensorId() const { return sensor_id; }
    std::string getOperator() const { return op; }
    double getValue() const { return value; }
    std::vector<std::string> getInvolvedSensors() const { return {sensor_id}; }

    // Destructor is just defaulted, as there are no resources to manage
    ~StepDifferenceRule() override = default;

  private:
    const std::string sensor_id;
    const std::string op;
    const double value;
    MeasDatabaseInterface *database =
        nullptr; // pointer to the measurement database
};

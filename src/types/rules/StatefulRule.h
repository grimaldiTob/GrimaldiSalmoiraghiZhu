#pragma once

#include "BaseRule.h"
#include "../../interfaces/MeasDatabaseInterface.h" 
#include <vector>
#include <unordered_map>

class StatefulRule : public BaseRule {
public:
    /**
     * @brief Constructor for StatefulRule.
     * @param rule_id Unique identifier for the rule.
     * @param priority Priority level of the rule (HIGH, MEDIUM, LOW).
     * @param sensor_id Identifier of the sensor to which the rule applies.
     * @param oprtor Operator for comparison (e.g., ">", "<", "==").
     * @param consecutive_meas Number of consecutive measurements required.
     * @param value Value to compare the sensor reading against.
     */
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
    std::optional<bool> evaluate(const TelemetryBatch& batch, 
        std::unordered_map<std::string, std::optional<bool>>& cache) override;

    // assigns the database interface to the pointer.
    void setMeasDatabase(MeasDatabaseInterface &db) { database = &db; }

    std::string getSensorId() const { return sensor_id; }
    std::string getOperator() const { return oprtor; }
    double getValue() const { return value; }
    double getConsecutiveMeas() const { return consecutive_meas; }
    std::vector<std::string> getInvolvedSensors() const { return {sensor_id}; }

private:
    const std::string sensor_id;
    const std::string oprtor;
    const double value;
    const double consecutive_meas;
    MeasDatabaseInterface* database = nullptr; // pointer to the measurement database
    // I am using a traditional, C-style pointer, since
    // BaseRule is not the owner of the database, which likely
    // needs no shared ownership nor automatic memory management,
    // since it should be created at program startup and destroyed at shutdown.
};
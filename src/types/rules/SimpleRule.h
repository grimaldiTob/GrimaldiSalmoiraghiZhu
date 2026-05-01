#ifndef SIMPLERULE_H
#define SIMPLERULE_H

#include "BaseRule.h"

class SimpleRule : public BaseRule {
public:
    SimpleRule(const std::string& rule_id, 
               RulePriority priority, 
               const std::string& sensor_id,
               const std::string& op,
               const std::string& value)
        : BaseRule(rule_id, RuleType::SIMPLE, priority), 
          sensor_id(sensor_id), 
          op(op), 
          value(value) {}

    // Ovverridden evaluate method (implementation in .cpp file)
    bool evaluate(const std::string& input) override;

    // TO BE DISCUSSED (see comment in BaseRule.h)
    std::string getSensorId() const { return sensor_id; }
    std::string getOperator() const { return op; }
    std::string getValue() const { return value; }

    // Destructor is just defaulted, as there are no resources to manage
    ~SimpleRule() override = default;
private:
    const std::string sensor_id;
    const std::string op;
    const std::string value;
};

#endif // SIMPLERULE_H


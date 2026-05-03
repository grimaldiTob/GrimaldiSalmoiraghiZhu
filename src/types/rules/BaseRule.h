#ifndef BASERULE_H
#define BASERULE_H

#include <string>
#include <optional>
#include <cmath> 
#include <unordered_map>
#include "../components/BatchAccumulator.h"

enum class RuleType {
    SIMPLE,
    STEP_DIFFERENCE,
    STATEFUL,
    CORRELATION
};

enum class RulePriority {
    LOW,
    MEDIUM,
    HIGH
};

class BaseRule {
public:
    BaseRule(const std::string& rule_id, 
             const RuleType type, 
             const RulePriority priority)
        : rule_id(rule_id), 
          type(type), 
          priority(priority) {}
    virtual ~BaseRule() = default;
    virtual std::optional<bool> evaluate(const BatchAccumulator& accumulator, 
        std::unordered_map<std::string, std::optional<bool>>& cache) = 0; 
    // I changed the return type to std::optional<bool> to allow for a "null" 
    // state in case of invalid input or other issues during evaluation.

    // TO BE DISCUSSED (see comment below)
    std::string getRuleId() const { return rule_id; }
    RuleType getType() const { return type; }
    RulePriority getPriority() const { return priority; }
   
protected: 
// I added these to the protected scope, but I am not so sure if this is the best way to do  it, 
// since some of our other components need some of them (e.g the priority) to properly function. 
// I added some functions to read (and read only) the members, but I am 
// open to alternative solutions. 
    const std::string rule_id;
    const RuleType type;
    const RulePriority priority; 
};

#endif // BASERULE_H
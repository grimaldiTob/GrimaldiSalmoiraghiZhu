#include <memory>
#include <string>
#include <unordered_map>
#include <optional>
#include "../interfaces/RuleEngineInterface.h"
#include "../interfaces/BatchProviderInterface.h"
#include "../../external/simdjson.h"
#include "../rules/BaseRule.h"
#include "../rules/SimpleRule.h"
#include "../rules/StepDifferenceRule.h"
#include "BatchAccumulator.h"

class RuleEngine : public RuleEngineInterface {

public:

    RuleEngine() = default;

    RuleEngine(std::shared_ptr<BatchProviderInterface> provider);

    void ruleParsing(simdjson::ondemand::parser& parser, const std::string &filename);

    void setProviderInterface(std::shared_ptr<BatchProviderInterface>);

    RulePriority parsePriority(std::string_view);

private:
    // vector in which we store all rules
    std::vector<BaseRule> rules_list;

    BatchAccumulator accumulator;

    // map in which we store the cache result for the evaluated rules
    // in particular key = "rule_id" --> value = boolean
    std::unordered_map<std::string, std::optional<bool>> rules_cache;
    // consider a method which resets the cache state when a batch gets evaluated
};
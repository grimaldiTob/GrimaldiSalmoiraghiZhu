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
#include "../rules/StatefulRule.h"
#include "../rules/LogicalCorrelationRule.h"
#include "BatchAccumulator.h"

class RuleEngine : public RuleEngineInterface {

public:

    /*
        Main design choice here is:
            Are we having a batchAccumulator with a reference to the ruleEngine,
            or a RuleEngine with a BatchAccumulator attribute?

            I am more for the second option since the ruleEngine is the logic of
            our program, having both the rules and the batch allows to evaluate 
            rules all in one structure (this is also the main Rule Engine use case)
    */
    RuleEngine() = default;

    RuleEngine(std::shared_ptr<BatchProviderInterface> provider);

    // use const obj& in order to avoid reallocating on each call
    const std::vector<std::shared_ptr<BaseRule>>& getRulesList() { return rules_list; };
    const BatchAccumulator& getAccumulator() { return accumulator; }
    const std::unordered_map<std::string, std::optional<bool>>& getRulesCache() { return rules_cache; };

    void ruleParsing(simdjson::ondemand::parser& parser, const std::string &filename);

    void setProviderInterface(std::shared_ptr<BatchProviderInterface>);

    RulePriority parsePriority(std::string_view);

    void resetCache();

private:
    // vector in which we store all rules
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    BatchAccumulator accumulator;

    // map in which we store the cache result for the evaluated rules
    // in particular key = "rule_id" --> value = boolean
    std::unordered_map<std::string, std::optional<bool>> rules_cache;
    // consider a method which resets the cache state when a batch gets evaluated
};
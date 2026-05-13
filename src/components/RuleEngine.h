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
#include "ThreadSafeBuffer.h"

class RuleEngine : public RuleEngineInterface {

public:

    explicit RuleEngine(ThreadSafeBuffer<TelemetryBatch>& broker)
        : m_broker(broker) {}

    // use const obj& in order to avoid reallocating on each call
    const std::vector<std::shared_ptr<BaseRule>>& getRulesList() const { return rules_list; };
    const std::unordered_map<std::string, std::optional<bool>>& getRulesCache() const { return rules_cache; };

    void setProviderInterface(std::shared_ptr<BatchProviderInterface>);

    // Protect the batch as read-only since the RuleEngine has to read and make evaluation without modify it
    void evaluateRules(const TelemetryBatch& batch);

    void resetCache();

    // The entry point for the Consumer Thread
    void run();
    
private:

    // The queue which the class will retrieve the batch from 
    ThreadSafeBuffer<TelemetryBatch>& m_broker;      

    // vector in which we store all rules
    std::vector<std::shared_ptr<BaseRule>> rules_list;
    

    // if we have exactly one consumer thread, keeping it as an attribute might work,
    // but can introduce hidden bugs into a multithreaded system (for parallelization)
    // I think is better to move it to a local variable in RuleEnginerun() and evaluateRules takes
    // as parameter the local batch
    //TelemetryBatch batch; 
    
    // map in which we store the cache result for the evaluated rules
    std::unordered_map<std::string, std::optional<bool>> rules_cache;

    // make it private since it is an helper internal to the class.
    RulePriority parsePriority(std::string_view);
};
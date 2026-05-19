#include <memory>
#include <string>
#include <unordered_map>
#include <optional>
#include <cstdint>

// EXTERNAL 
#include "../../external/simdjson.h"

// RULES 
#include "../types/rules/BaseRule.h"
#include "../types/rules/SimpleRule.h"
#include "../types/rules/StepDifferenceRule.h"
#include "../types/rules/StatefulRule.h"
#include "../types/rules/LogicalCorrelationRule.h"

// INTERFACES
#include "../interfaces/ConsumerBuffer.h"
#include "../interfaces/RuleEngineInterface.h"

/* I don't think including just the header file of the interfaces is enough, 
 since their methods are virtual and we need to call them in the RuleEngine class. 
 What I have dpne, I have forward-declared the interfaces in the header file 
 and included their headers in the cpp file.
*/
#include "../interfaces/RuleEngineInterface.h"

// Forward-declare interfaces here to reduce header coupling.
class RuleLoaderInterface;
class MeasDatabaseInterface;
class OutputDispatcherInterface;

class RuleEngine : public RuleEngineInterface {

public:

    // Constructor that instanciates all necessary interfaces 
    RuleEngine(ConsumerBuffer<TelemetryBatch>& broker,
                       MeasDatabaseInterface& db,
                       std::optional<int64_t> initialTimestamp)
                       : m_broker(broker),
                         db(db),
                         m_evaluationTimestamp(initialTimestamp){}

        explicit RuleEngine(ConsumerBuffer<TelemetryBatch>& broker,
                                                MeasDatabaseInterface& db,
                                                std::optional<int64_t> initialTimestamp)
                : m_evaluationTimestamp(initialTimestamp),
                    m_broker(broker),
                    db(db) {}

    // use const obj& in order to avoid reallocating on each call
    const std::vector<std::shared_ptr<BaseRule>>& getRulesList() const { return rules_list; };
    const std::unordered_map<std::string, std::optional<bool>>& getRulesCache() const { return rules_cache; };

    void setOutputDispatcher(OutputDispatcherInterface& dispatcher) { m_outputDispatcher = &dispatcher; }

    // Protect the batch as read-only since the RuleEngine has to read and make evaluation without modify it
    void evaluateRules(const TelemetryBatch& batch)  override;

    // ideally we can also think of having the rule loader as a class attribute but 
    // this has just a one shot usage. (after loading is just wasted memory)
    void setRulesList(RuleLoaderInterface& loader,
                  simdjson::ondemand::parser& parser);

    void checkRuleResult();

    void storeBatchMeasurements(const TelemetryBatch &batch);

    void resetCache();

    // The entry point for the Consumer Thread
    void run();
    
private:
    const std::string RULES_FILENAME = "rules.json";
    std::optional<int64_t> m_evaluationTimestamp;

    // The queue which the class will retrieve the batch from 
    ConsumerBuffer<TelemetryBatch>& m_broker;

    MeasDatabaseInterface& db;

    // vector in which we store all rules
    std::vector<std::shared_ptr<BaseRule>> rules_list;
    

    // if we have exactly one consumer thread, keeping it as an attribute might work,
    // but can introduce hidden bugs into a multithreaded system (for parallelization)
    // I think is better to move it to a local variable in RuleEnginerun() and evaluateRules takes
    // as parameter the local batch
    //TelemetryBatch batch; 
    
    // map in which we store the cache result for the evaluated rules
    std::unordered_map<std::string, std::optional<bool>> rules_cache;

    OutputDispatcherInterface* m_outputDispatcher = nullptr;

    // make it private since it is an helper internal to the class.
    RulePriority parsePriority(std::string_view);
};
#pragma once

#include <cstdint>
#include <memory>
#include <mpi.h>
#include <optional>
#include <string>
#include <unordered_map>

// EXTERNAL
#include "../../external/simdjson.h"

// RULES
#include "../types/rules/BaseRule.h"
#include "../types/rules/LogicalCorrelationRule.h"
#include "../types/rules/SimpleRule.h"
#include "../types/rules/StatefulRule.h"
#include "../types/rules/StepDifferenceRule.h"

// INTERFACES
#include "../interfaces/ConsumerBuffer.h"
#include "../interfaces/RuleEngineInterface.h"

// Forward-declare interfaces here to reduce header coupling.
class RuleLoaderInterface;
class MeasDatabaseInterface;
class OutputDispatcherInterface;

class RuleEngine : public RuleEngineInterface {

  public:
    explicit RuleEngine(ConsumerBuffer<TelemetryBatch> &broker,
                        MeasDatabaseInterface &db,
                        OutputDispatcherInterface &outputDispatcher,
                        std::optional<int64_t> initialTimestamp)
        : m_evaluationTimestamp(initialTimestamp), m_broker(broker), db(db),
          m_outputDispatcher(outputDispatcher) {}

    virtual ~RuleEngine() = default;

    // use const obj& in order to avoid reallocating on each call
    const std::vector<std::shared_ptr<BaseRule>> &getRulesList() const {
        return rules_list;
    };

    const std::unordered_map<std::string, std::optional<bool>> &
    getRulesCache() const {
        return rules_cache;
    };

    void setRulesFilename(const std::string &filename) {
        rules_filename = filename;
    }

    // Protect the batch as read-only since the RuleEngine has to read and
    // make evaluation without modify it
    virtual void evaluateRules(const TelemetryBatch &batch) override;

    // Send a request to load rules from RuleLoader
    void setRulesList(RuleLoaderInterface &loader);

    void checkRuleResult();

    void storeBatchMeasurements(const TelemetryBatch &batch);

    void resetCache();

    // The entry point for the Consumer Thread
    virtual void run();

  private:
    std::string rules_filename;

    MeasDatabaseInterface &db;

    OutputDispatcherInterface &m_outputDispatcher;

    /**
     * By setting the below attribus with protected flag we
     * are enabling those to be accesible by  the derived classes
     * like MpiRuleEngine
     *  */
  protected:
    std::optional<int64_t> m_evaluationTimestamp;

    // The queue which the class will retrieve the batch from
    ConsumerBuffer<TelemetryBatch> &m_broker;

    // vector in which we store all rules
    std::vector<std::shared_ptr<BaseRule>> rules_list;

    // map in which we store the cache result for the evaluated rules
    std::unordered_map<std::string, std::optional<bool>> rules_cache;
};

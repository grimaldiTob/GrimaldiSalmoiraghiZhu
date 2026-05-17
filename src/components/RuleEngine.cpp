#include "RuleEngine.h"
#include <fstream>
// Include concrete interface headers here so header can forward-declare them.
#include "../interfaces/BatchProviderInterface.h"
#include "../interfaces/RuleLoaderInterface.h"
#include "../interfaces/MeasDatabaseInterface.h"
#include "../interfaces/OutputDispatcherInterface.h"

/** @brief 
 * Reset the content of the cache erasing just the values not the key.
*/
void RuleEngine::resetCache() {
    // ho trovato questa funzione ed è troppo in stile Formaggia
    // Luca: e ovviamente come tutte le funzioni incomprensibili in 
    // stile Formaggia c'era un bel problema di binging una non-const
    // reference a un elemento della mappa, che è stato risolto con 
    // value_or(), che invece restituisce una copia del valore 
    // (e quindi std::optional<bool> invece che una reference).
    std::for_each(rules_cache.begin(), rules_cache.end(),
                  [](auto &p){
                      p.second = std::nullopt;
                  });
}

/** @brief method that populates the rules_list calling the loadRules()
 * method through the loader interface object.
 */
void RuleEngine::setRulesList(RuleLoaderInterface& loader,
                              simdjson::ondemand::parser& parser) {
    rules_list.clear();
    rules_cache.clear(); // avoid stale cache entries
    loader.loadRules(parser, RULES_FILENAME, rules_list);

    for(auto& rule : rules_list) {
        if(rule->getType() == RuleType::STATEFUL) {
            auto* stateful_rule = dynamic_cast<StatefulRule *>(rule.get());
            stateful_rule->setMeasDatabase(db);
        }
    }
}

void RuleEngine::checkRuleResult() {
    bool all_true = true;
    // std::unordered_map<std::string, std::optional<bool>> failed_rules;
    std::vector<std::shared_ptr<BaseRule>> failed_rules;
    failed_rules.reserve(rules_list.size()); // reserve hypotetically the size of the entire cache size

    // iterate in the rule list and 
    for (auto& rule : rules_list) {
        const std::string& rule_id = rule->getRuleId();
        auto it = rules_cache.find(rule_id); // find returns the pointer to the entry associated to the rule_id
        if (it == rules_cache.end() || !it->second.value_or(false)) { // value_or() used for std::optional<bool>
            all_true = false;
            // failed_rules.emplace(rule, it == rules_cache.end() ? std::nullopt : it->second); 
            failed_rules.emplace_back(rule); 
            // if the rule was not found return std::nullopt otherwise the value (either false or std::nullopt again) 
        }
    }

    if (!m_outputDispatcher) {
        return;
    }

    if (all_true) {
        m_outputDispatcher->appendValidData(db);
        return;
    }

    m_outputDispatcher->appendAlarms(db, failed_rules);
}

void RuleEngine::storeBatchMeasurements(const TelemetryBatch& batch) {
    for (size_t i = 0; i < batch.values.size();++i) {
        db.storeResult(batch.sensors_name[i], batch.values[i]);
    }
}

/** @brief Method which evaluates all the rules stored in the rules_list
 * For each rule checks if there is a match between the measurement in the
 * batch and evaluates the rule
 */
void RuleEngine::evaluateRules(const TelemetryBatch& batch) {
    for(auto& rule : rules_list) {
        const std::string& rule_id = rule->getRuleId();
        auto result = rule->evaluate(batch, rules_cache);
        rules_cache[rule_id] = result;
        
        // also thinking about parallelization of rule evaluation
        // we cannot really parallelize the whole rules_list for two reasons:
            // 1. the cache becomes useless
            // 2. (and most important) WE HAVE PRIORITIES
        
        // so we can think of like parallelizing over 'HIGH', 'MEDIUM', 'LOW' priorities but not all of them.
    }
    storeBatchMeasurements(batch);
}

/** @brief this method runs the whole loop of our application.
 * The call to pop() method ensures that there is at least one
 * element in the queue. This makes the loop efficient and 
 * coherent with the data stream received from the broker.
 */
void RuleEngine::run() {
    TelemetryBatch currentBatch;

    // This loop should handle all the complex multithreading logic.
    while(m_broker.pop(currentBatch)) {
        TelemetryBatch subBatch(currentBatch.getSize());
        int64_t activeTimestamp = m_evaluationTimestamp.value_or(0);

        for (size_t i = 0; i < currentBatch.getSize(); ++i) {
            // if the timestamp changes we need to change the active timestamp
            if (currentBatch.timestamps[i] != activeTimestamp) {
                /*
                    If the timestamp changes we evaluate the measurements collected so far,
                    check the cache content and print values in output dispatcher and in the end 
                    reset the cache values. THIS METHOD CAN HAVE MANY PROBLEMS, but for now it works
                    fine in my head.
                */
                m_evaluationTimestamp = activeTimestamp;
                evaluateRules(subBatch);
                checkRuleResult();
                resetCache();
                subBatch.clear();
                activeTimestamp = currentBatch.timestamps[i];
            }
            // collect the measurement in the currentBatch
            subBatch.emplaceBack(
                currentBatch.sensors_name[i],
                currentBatch.timestamps[i],
                currentBatch.values[i],
                currentBatch.priorities[i]
            );
        }
        // in the end we evaluate the remaining measurements in the batch
        if (subBatch.getSize() > 0) {
            m_evaluationTimestamp = activeTimestamp;
            evaluateRules(subBatch);
        }
    }
}
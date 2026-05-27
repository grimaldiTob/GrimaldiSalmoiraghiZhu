#include "RuleEngine.h"
#include <fstream>
// Include concrete interface headers here so header can forward-declare them.
#include "../interfaces/BatchProviderInterface.h"
#include "../interfaces/MeasDatabaseInterface.h"
#include "../interfaces/OutputDispatcherInterface.h"
#include "../interfaces/RuleLoaderInterface.h"

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
                  [](auto &p) { p.second = std::nullopt; });
}

/** @brief method that populates the rules_list calling the loadRules()
 * method through the loader interface object.
 */
void RuleEngine::setRulesList(RuleLoaderInterface &loader) {
    rules_list.clear();
    rules_cache.clear(); // avoid stale cache entries
    loader.loadRules(rules_filename, rules_list);

    for (auto &rule : rules_list) {
        if (rule->getType() == RuleType::STATEFUL) {
            auto *stateful_rule = dynamic_cast<StatefulRule *>(rule.get());
            stateful_rule->setMeasDatabase(db);
        }
        if (rule->getType() == RuleType::STEP_DIFFERENCE) {
            auto *step_diff_rule =
                dynamic_cast<StepDifferenceRule *>(rule.get());
            step_diff_rule->setMeasDatabase(db);
        }
    }
}

void RuleEngine::checkRuleResult() {
    bool all_true = true;
    // std::unordered_map<std::string, std::optional<bool>> failed_rules;
    std::vector<std::shared_ptr<BaseRule>> failed_rules;
    failed_rules.reserve(
        rules_list
            .size()); // reserve hypotetically the size of the entire cache size

    // iterate in the rule list and
    for (auto &rule : rules_list) {
        const std::string &rule_id = rule->getRuleId();
        auto it = rules_cache.find(rule_id); // find returns the pointer to the
                                             // entry associated to the rule_id
        if (it == rules_cache.end() ||
            !it->second.value_or(
                false)) { // value_or() used for std::optional<bool>
            all_true = false;
            // failed_rules.emplace(rule, it == rules_cache.end() ? std::nullopt
            // : it->second);
            failed_rules.emplace_back(rule);
            // if the rule was not found return std::nullopt otherwise the value
            // (either false or std::nullopt again)
        }
    }

    if (all_true) {
        m_outputDispatcher.appendValidData(db, m_evaluationTimestamp);
        return;
    }

    m_outputDispatcher.appendAlarms(db, failed_rules, m_evaluationTimestamp);
}

void RuleEngine::storeBatchMeasurements(const TelemetryBatch &batch) {
    for (size_t i = 0; i < batch.values.size(); ++i) {
        db.storeResult(batch.sensors_name[i], batch.values[i]);
    }
}

/** @brief Method which evaluates all the rules stored in the rules_list
 * For each rule checks if there is a match between the measurement in the
 * batch and evaluates the rule
 */
void RuleEngine::evaluateRules(const TelemetryBatch &batch) {

    // define a lambda function which will be called inside the function itself.
    auto evaluatePriorityGroup = [&](RulePriority priority) {
        std::vector<std::pair<std::string, std::optional<bool>>> results;
        results.reserve(rules_list.size());

// Evaluate non-correlation rules in parallel and merge results after the
// barrier.
#pragma omp parallel
        {
            std::vector<std::pair<std::string, std::optional<bool>>>
                local_results;

// RONZANI FIERO DEL MIO DYNAMIC SCHEDULING
#pragma omp for schedule(dynamic)
            for (size_t i = 0; i < rules_list.size(); ++i) {
                auto &rule = rules_list[i];
                if (rule->getPriority() != priority) {
                    continue;
                }
                const std::string &rule_id = rule->getRuleId();
                auto result = rule->evaluate(batch, rules_cache);
                local_results.emplace_back(rule_id, result);

                /*
                    OK HERE WE HAVE A HUGE PROBLEM, I'LL TRY TO EXPLAIN:
                        Basically If we evaluate a correlation rule, we are
                   accessing the cache and storing the results of the
                   intermediate rules. But we cant do it in a parallel
                   environment since it would mean race condition in the cache.

                        So I tough, ok we do not store the intermediate result,
                   but in that case we have another big problem: Some rules
                   modify the state of the Rule Itself (like the step difference
                   rule). This means that evaluating a rule two times (once for
                   the correlation and one for the rule itself) generates
                   different result.

                        SOLVED:
                            now rules are completely stateless, which makes it
                   possible for us to evaluate all the rules in parallel. Since
                   we cannot store results in the cache after each evaluation,
                   if a correlation rule and one simple rule associated to it
                            have both the same priority, the simple rule will be
                   evaluated twice, but this is a logical limitation of our
                   program that we cannot overcome.

                */
            }

#pragma omp critical
            results.insert(results.end(), local_results.begin(),
                           local_results.end());
            // inserts the local results accumulated from the end.
        }

        for (auto &entry : results) {
            rules_cache[entry.first] = entry.second;
        }
    };
    // evaluate based on Priority order
    evaluatePriorityGroup(RulePriority::HIGH);
    evaluatePriorityGroup(RulePriority::MEDIUM);
    evaluatePriorityGroup(RulePriority::LOW);

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
    while (m_broker.pop(currentBatch)) {
        TelemetryBatch subBatch(currentBatch.getSize());
        int64_t activeTimestamp = m_evaluationTimestamp.value_or(0);

        for (size_t i = 0; i < currentBatch.getSize(); ++i) {
            // if the timestamp changes we need to change the active timestamp
            if (currentBatch.timestamps[i] != activeTimestamp) {
                /*
                    If the timestamp changes we evaluate the measurements
                   collected so far, check the cache content and print values in
                   output dispatcher and in the end reset the cache values. THIS
                   METHOD CAN HAVE MANY PROBLEMS, but for now it works fine in
                   my head.
                */
                m_evaluationTimestamp = activeTimestamp;

                evaluateRules(subBatch);
                checkRuleResult();
                resetCache();
                // serialEvaluate(subBatch);
                subBatch.clear();
                activeTimestamp = currentBatch.timestamps[i];
            }
            // collect the measurement in the currentBatch
            subBatch.emplaceBack(
                currentBatch.sensors_name[i], currentBatch.timestamps[i],
                currentBatch.values[i], currentBatch.priorities[i]);
        }
        // in the end we evaluate the remaining measurements in the batch
        if (subBatch.getSize() > 0) {
            m_evaluationTimestamp = activeTimestamp;
            evaluateRules(subBatch);
        }
    }
    checkRuleResult();
    resetCache();
}

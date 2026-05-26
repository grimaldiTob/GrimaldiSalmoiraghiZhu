#include "RuleEngine.h"
#include <fstream>
// Include concrete interface headers here so header can forward-declare them.
#include "../interfaces/BatchProviderInterface.h"
#include "../interfaces/MeasDatabaseInterface.h"
#include "../interfaces/OutputDispatcherInterface.h"
#include "../interfaces/RuleLoaderInterface.h"

#ifndef USE_MPI_RULE_ENGINE
#define USE_MPI_RULE_ENGINE 0
#endif

/**
 * Couple of helper functions defined here in order to handle
 * MPI massage passing correctly.
 *
 * Since MPI does not support passing std::optional<bool>
 * we encode it in an integer.
 */
static inline int8_t encode(std::optional<bool> v) {
    if (!v.has_value())
        return -1;
    return *v ? 1 : 0;
}
static inline std::optional<bool> decode(int8_t v) {
    if (v < 0)
        return std::nullopt;
    return v == 1;
}

struct RuleEvalMsg {
    int32_t idx;
    int8_t value;
};

/** Broadcast of Telemetry Batch Data across processors using MPI.
 * It is important to notice that since MPI does not support the string object,
 * but just the char we had to create a custom string with all the sensors name
 * concatenated.
 *
 */
static void broadcastTelemetryBatch(TelemetryBatch &batch, int root,
                                    MPI_Comm comm) {
    int rank;
    MPI_Comm_rank(comm, &rank);

    // broadcast the batch size first of all
    int32_t n = static_cast<int32_t>(batch.getSize());
    MPI_Bcast(&n, 1, MPI_INT32_T, root, comm);

    if (n == 0)
        return; // if batch is emptu

    // Resize on non-root rank
    if (batch.getSize() != static_cast<size_t>(n)) {
        batch.sensors_name.resize(n);
        batch.timestamps.resize(n);
        batch.values.resize(n);
        batch.priorities.resize(n);
    }

    // Broadcast numeric arrays
    MPI_Bcast(batch.timestamps.data(), n, MPI_INT64_T, root, comm);
    MPI_Bcast(batch.values.data(), n, MPI_DOUBLE, root, comm);
    MPI_Bcast(batch.priorities.data(), n, MPI_INT, root, comm);

    // Broadcast strings (pack lengths + char buffer)
    std::vector<int32_t> lengths(n);
    std::string packed;

    // do the packing only in the root node.
    if (rank == root) {
        size_t total = 0;
        for (int i = 0; i < n; ++i) {
            lengths[i] = static_cast<int32_t>(batch.sensors_name[i].size());
            total += lengths[i];
        }
        packed.reserve(total);
        for (int i = 0; i < n; ++i) {
            packed.append(batch.sensors_name[i]);
        }
    }
    MPI_Bcast(lengths.data(), n, MPI_INT, root, comm);

    int32_t total_len = 0;
    for (int i = 0; i < n; ++i)
        total_len += lengths[i];
    packed.resize(total_len);
    MPI_Bcast(packed.data(), total_len, MPI_CHAR, root, comm);

    // Unpack on non-root
    if (int rank; MPI_Comm_rank(comm, &rank), rank != root) {
        size_t offset = 0;
        for (int i = 0; i < n; ++i) {
            batch.sensors_name[i] = packed.substr(offset, lengths[i]);
            offset += lengths[i];
        }
    }
}

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
void RuleEngine::setRulesList(RuleLoaderInterface &loader,
                              simdjson::ondemand::parser &parser) {
    rules_list.clear();
    rules_cache.clear(); // avoid stale cache entries
    loader.loadRules(parser, rules_filename, rules_list);

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

void RuleEngine::serialEvaluate(const TelemetryBatch &batch) {
    for (size_t i = 0; i < rules_list.size(); ++i) {
        auto &rule = rules_list[i];
        std::optional<bool> result = rule->evaluate(batch, rules_cache);
        rules_cache[rule->getRuleId()] = result;
    }
    storeBatchMeasurements(batch);
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

/** @brief Implementation of parallelization using MPI.
 * Workflow is the following:
 *     - Initialize MPI variables
 *     - Broadcast the telemetry batch across all processors
 *     - Compute evaluation indexes with remainder
 *     - each processor evaluates different rules (with OpenMP)
 *     - Each processor Gathers local results together in the cache
 *
 * In the end just the root processor check the rule results, while all
 * processors clean their cache content.
 */
void RuleEngine::evaluateRulesMPI(TelemetryBatch &batch, MPI_Comm comm) {
    int rank = 0, size = 1;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    // Batch comes from rank 0 only
    broadcastTelemetryBatch(batch, 0, comm);

    auto evalGroup = [&](RulePriority priority) {
        std::vector<RuleEvalMsg> local_results;

        int total_rules = rules_list.size();
        int chunk_size = total_rules / size;
        int remainder = total_rules % size;

        // compute the number of rules to assign to each rank
        int start_idx = rank * chunk_size + std::min(rank, remainder);
        int end_idx =
            start_idx + chunk_size +
            (rank < remainder
                 ? 1
                 : 0); // chunk size + 1 for the first remainder processes

#pragma omp parallel
        {
            std::vector<RuleEvalMsg> thread_results;
#pragma omp for schedule(dynamic) // again ronzzani fiero di me
            for (size_t k = start_idx; k < end_idx; ++k) {
                auto &rule = rules_list[k];
                if (rule->getPriority() != priority)
                    continue;

                auto result =
                    rule->evaluate(batch, rules_cache); // read-only cache

                // here we need to use integer values to address results
                // encode() map the boolean result to an integer
                thread_results.push_back(
                    {static_cast<int32_t>(k), encode(result)});
            }
#pragma omp critical
            local_results.insert(local_results.end(), thread_results.begin(),
                                 thread_results.end());
        }

        // Gather sizes and data from all ranks
        int local_count = static_cast<int>(local_results.size());
        std::vector<int> counts(size), displacements(size);

        // places the local count inside the counts at the right index given the
        // rank
        MPI_Allgather(&local_count, 1, MPI_INT, counts.data(), 1, MPI_INT,
                      comm);

        int total = 0;
        for (int i = 0; i < size; ++i) {
            displacements[i] = total;
            total += counts[i];
        }

        // take the number of bytes of each single RuleEvalMsg
        const int elem_size = static_cast<int>(sizeof(RuleEvalMsg));
        std::vector<int> byte_counts(size), byte_displs(size);
        for (int i = 0; i < size; ++i) {
            // compute the number of bytes which will be
            byte_counts[i] = counts[i] * elem_size;
            byte_displs[i] = displacements[i] * elem_size;
        }

        // vector of all the aggregated evaluated rules
        std::vector<RuleEvalMsg> all_results(total);

        /* Here in the AllGatherv we are sending bytes (defining a new MPI
        struct was a mess) So instead of passing counts and displacement in the
        form of integers (elements) we pass the corresponding number of bytes we
        want to transfers
        */
        MPI_Allgatherv(local_results.data(), local_count * elem_size, MPI_BYTE,
                       all_results.data(), byte_counts.data(),
                       byte_displs.data(), MPI_BYTE, comm);

        // Update cache consistently on every rank
        for (const auto &msg : all_results) {
            const std::string &id = rules_list[msg.idx]->getRuleId();
            rules_cache[id] = decode(msg.value);
        }
    };

    evalGroup(RulePriority::HIGH);
    evalGroup(RulePriority::MEDIUM);
    evalGroup(RulePriority::LOW);

    // Replicate DB state (needed by stateful rules)
    storeBatchMeasurements(batch);

    // Only rank 0 should go in output
    if (rank == 0) {
        checkRuleResult();
        resetCache();
    } else {
        resetCache();
    }
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
#if USE_MPI_RULE_ENGINE
                evaluateRulesMPI(subBatch, MPI_COMM_WORLD);
#else
                evaluateRules(subBatch);
                checkRuleResult();
                resetCache();
#endif
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
#if USE_MPI_RULE_ENGINE
            evaluateRulesMPI(subBatch, MPI_COMM_WORLD);
#else
            evaluateRules(subBatch);
            checkRuleResult();
            resetCache();
#endif
        }
    }
}

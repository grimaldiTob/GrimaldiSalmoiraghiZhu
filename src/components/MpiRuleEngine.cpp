#include "MpiRuleEngine.h"

static void broadcastTelemetryBatch(TelemetryBatch &batch, int root,
                                    MPI_Comm comm) {
    int rank;
    MPI_Comm_rank(comm, &rank);

    // broadcast the batch size first of all
    int32_t n = static_cast<int32_t>(batch.getSize());
    MPI_Bcast(&n, 1, MPI_INT32_T, root, comm);

    if (n == 0)
        return; // if batch is empty

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

void MpiRuleEngine::evaluateRules(const TelemetryBatch &batch) {

    int rank = 0, size = 1;
    MPI_Comm_rank(m_comm, &rank);
    MPI_Comm_size(m_comm, &size);

    // Since the broadcastTelemetryBatch() method will modify the batch
    // parameter, we need to create a copy of the batch received by the
    // evaluateRules()
    TelemetryBatch tmp = batch;

    // Batch comes from rank 0 only
    broadcastTelemetryBatch(tmp, 0, m_comm);

    const std::vector<std::shared_ptr<BaseRule>> &rule_list = getRulesList();

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
                      m_comm);

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
                       byte_displs.data(), MPI_BYTE, m_comm);

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
}

void MpiRuleEngine::run() {
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

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <thread>

#include "TelemetryBatch.h"
#include "BatchFile.h"
#include "ThreadSafeBuffer.h"
#include "../interfaces/BatchProviderInterface.h"
#include "../interfaces/BatchAccumulatorInterface.h"
#include "../interfaces/RuleEngineInterface.h"

/**
 * @brief Accumulates valid packets into a local batch for processing.
 * Implements BatchAccumulatorInterface for ingestion and BatchProviderInterface for retrieval.
 */
class BatchAccumulator : public BatchAccumulatorInterface {

public:

    /**
     * @brief Constructor 
     */
    BatchAccumulator::BatchAccumulator(ThreadSafeBuffer<TelemetryBatch> buffer, size_t batchSize = 100) 
        : m_buffer(buffer),
          m_batchSize(batchSize) {}

    // Overloading operator() allows the object to be passed directly to std::thread
    void operator()() {

        m_buffer.push(m_batchFile);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_buffer.finish_production();   

    }
    
    /*============ GETTER ============*/
    const size_t          getBatchSize() const;
    const TelemetryBatch& getBatchFile() const;

    /*========================== SETTER ============================*/
    void setEvaluator(std::shared_ptr<RuleEngineInterface> evaluator);

    /**
     * @brief Store a validated telemetry batch into the accumulator.
     *
     * This method accepts a batch of telemetry data that has already been validated
     * by the DataIngestor and accumulates it internally. Once the batch reaches the
     * configured size threshold, the accumulated batch becomes
     * available for processing by the rule engine.
     */
    void storeValidData(TelemetryBatch &validBatch) override;
    
    /**
     * @brief TODO
     */
    const std::unordered_map<std::string, std::vector<double>>& getMeasurementsHistory() const;

    /**
     * @brief Saving results
     * methods that clears the batch when it finishes evaluation and stores result in the history
     */
    void storeResultHistory();
    
    /** 
     * @brief Accumulate the batch 
     */
    void accumulate(const TelemetryBatch& batch);

private:

    bool checkBatchSize(size_t addedSize) const;
    size_t getOverflowSize(size_t addedSize) const;
    void sortPriorities();

    /* ============================= ATTRIBUTE ====================*/
    size_t                                               m_batchSize;            // used to check wheter TelemetryBatch 
    TelemetryBatch                                       m_batchFile;            // the current batch
    TelemetryBatch                                       m_batchTmp;             // the temporal batch
    std::shared_ptr<RuleEngineInterface>                 m_evaluator;            // interface provided by the RuleEngine class

    // std::unordered_map<std::string, std::vector<double>> m_measurementsHistory;  // including the measurement history in the BatchAccumulator;
    ThreadSafeBuffer<TelemetryBatch>&                    m_buffer;               // buffer/queue to store batches that ary ready to be processed 

};   
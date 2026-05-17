#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <thread>
#include <string>

#include "TelemetryBatch.h"
#include "BatchFile.h"
#include "ThreadSafeBuffer.h"
#include "../interfaces/BatchProviderInterface.h"
#include "../interfaces/BatchAccumulatorInterface.h"
#include "../interfaces/RuleEngineInterface.h"
#include "../interfaces/MeasDatabaseInterface.h"

/**
 * @brief Accumulates valid packets into a local batch for processing.
 * Implements BatchAccumulatorInterface for ingestion and BatchProviderInterface for retrieval.
 */
class BatchAccumulator : public BatchAccumulatorInterface {

public:

    /** @brief Constructor */
    explicit BatchAccumulator(ThreadSafeBuffer<TelemetryBatch>& broker, 
                              size_t batchSize = 100) 
        : m_broker(broker),
          m_batchSize(batchSize) {}
    
    /*================= GETTER ==============*/
    size_t                getBatchSize() const;
    const TelemetryBatch& getBatchFile() const;
    const TelemetryBatch& getBatchTmp()  const;

    /*========================*/

    /** @brief Store a validated telemetry batch into the accumulator.
     *
     * This method accepts a batch of telemetry data that has already been validated
     * by the DataIngestor and accumulates it internally. Once the batch reaches the
     * configured size threshold, the accumulated batch becomes
     * available for processing by the rule engine.
     */
    void storeValidData(TelemetryBatch &validBatch) override;

 private:   
 
    /* =============== INTERNAL METHODS ===================*/
    // bool checkBatchSize() const;           // Return true if the new size of the batch is greather than m_batcSize
    // size_t getOverflowSize(size_t addedSize) const;        // Return the number of elements that are more 
    // void storeResult(std::string id, double value) const;  // Store the results into the m_database
    // void accumulate(int start);          // Accumulate the batch into m_batchFile
    
    /* ============================= ATTRIBUTE ===========*/
    size_t                                m_batchSize;    // used to check wheter TelemetryBatch 
    TelemetryBatch                        m_batchFile;    // the current batch
    ProducerBuffer<TelemetryBatch>&       m_broker;     // buffer/queue to store batches that ary ready to be processed (will be uncomment once included the queue)
    //TelemetryBatch                        m_batchTmp;     // better keep track of the cache
    // RuleEngineInterface&                  m_evaluator;    // interface to trigger the RuleEngine.evaluation() (will be discarded once included the queue)
    // MeasDatabaseInterface&                m_database;     // interface to store data into the MeasDatabase class

};   
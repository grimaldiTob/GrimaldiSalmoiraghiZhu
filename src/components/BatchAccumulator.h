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
#include "../interfaces/MeasDatabaseInterface.h"

/**
 * @brief Accumulates valid packets into a local batch for processing.
 * Implements BatchAccumulatorInterface for ingestion and BatchProviderInterface for retrieval.
 */
class BatchAccumulator : public BatchAccumulatorInterface {

public:

    // This will be the final constructor but at the moment for the serial implementation
    // the BatchAccumulator keeps the RuleEngine interface to invoke rule evaluation processing
    // /** @brief Constructor */
    // BatchAccumulator::BatchAccumulator(ThreadSafeBuffer<TelemetryBatch>& buffer, size_t batchSize = 100) 
    //     : m_buffer(buffer),
    //       m_batchSize(batchSize) {}

    /** @brief Constructor used for the serial basic implementation */
    BatchAccumulator::BatchAccumulator(RuleEngineInterface& evaluator, MeasDatabaseInterface database, size_t batchSize = 100)
        : m_evaluator(evaluator),
          m_database(database),
          m_batchSize(batchSize) {}

    // // Overloading operator() allows the object to be passed directly to std::thread
    // void operator()() {

    //     m_buffer.push(m_batchFile);
    //     std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //     m_buffer.finish_production();   

    // }
    
    /*============ GETTER ============*/
    const size_t          getBatchSize() const;
    const TelemetryBatch& getBatchFile() const;

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
    bool checkBatchSize(size_t addedSize) const;           // Return true if the new size of the batch is greather than m_batcSize
    size_t getOverflowSize(size_t addedSize) const;        // Return the number of elements that are more 
    void storeResult(std::string id, double value) const;  // Store the results into the m_database
    void accumulate(const TelemetryBatch& batch);          // Accumulate the batch into m_batchFile
    
    /* ============================= ATTRIBUTE ===========*/
    size_t                                m_batchSize;    // used to check wheter TelemetryBatch 
    TelemetryBatch                        m_batchFile;    // the current batch
    RuleEngineInterface&                  m_evaluator;    // interface to trigger the RuleEngine.evaluation() (will be discarded once included the queue)
    MeasDatabaseInterface&                m_database;     // interface to store data into the MeasDatabase class
    //ThreadSafeBuffer<TelemetryBatch>&     m_buffer;     // buffer/queue to store batches that ary ready to be processed (will be uncomment once included the queue)

};   
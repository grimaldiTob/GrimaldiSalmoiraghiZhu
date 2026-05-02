#pragma once

#include <memory>
#include<vector>

#include "TelemetryBatch.h"
#include "BatchFile.h"
#include "../interfaces/BatchProviderInterface.h"
#include "../interfaces/BatchAccumulatorInterface.h"
#include "../interfaces/RuleEngineInterface.h"

/**
 * @brief Accumulates valid packets into a local batch for processing.
 * Implements BatchAccumulatorInterface for ingestion and BatchProviderInterface for retrieval.
 */

class BatchAccumulator : public BatchAccumulatorInterface, public BatchProviderInterface {

public:

    BatchAccumulator() = default;

    BatchAccumulator(size_t batchSize = 100);

    BatchAccumulator(std::shared_ptr<RuleEngineInterface> ruleEngine, size_t batchSize);
        
    // From BatchAccumulatorInterface
    void storeValidData(TelemetryBatch&) override;
        
    // From BatchProviderInterface
    TelemetryBatch getBatchFile() override;

    void setRuleEngineInterface(RuleEngineInterface&);
        
private:
    // are we accumulating batches or files? I would extract the single packets from the Telemetry Batch 
    // and store them in priority order in other structure "Accumulator" (???)
    // std::vector<TelemetryBatch m_batchFile;
    // TelemetryBatch m_batchFile; // no point in having a a vector of Telemetrybatch since it is a group of vectors
    // we can store all the packets received from the Ingestor in this member until we dont reach the batch size 
    // and in the end sort all the packets considering the priority value.

    TelemetryBatch                        m_batchFile;
    std::shared_ptr<RuleEngineInterface> m_ruleEngine;
    size_t                                m_batchSize; // used to check wheter TelemetryBatch reached the limit or not            
        
    void accumulate(TelemetryBatch);
        
    bool checkBatchSize() const;
        
    void notifyBatchAvailability();

    void sortPriorities();
};   
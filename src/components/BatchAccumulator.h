#pragma once

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
    BatchAccumulator(std::shared_ptr<RuleEngineInterface> ruleEngine);
        
    // From BatchAccumulatorInterface
    void sendValidData(TelemetryBatch) override;
        
    // From BatchProviderInterface
    std::vector<TelemetryBatch> getBatchFile() override;
        
private:
    std::vector<TelemetryBatch> m_batchFile;
    std::shared_ptr<RuleEngineInterface> m_ruleEngine;
    size_t m_batchSize;                       
        
    void accumulate(TelemetryBatch);
        
    bool checkBatchSize() const;
        
    void notifyBatchAvailability();    
    
};   
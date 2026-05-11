#include "BatchAccumulator.h"

const size_t BatchAccumulator::getBatchSize() const {
    return m_batchSize;
}

// modified the method so it is not returning by value the whole batch 
const TelemetryBatch& BatchAccumulator::getBatchFile() const {
    return m_batchFile;
}

void BatchAccumulator::setEvaluator(std::shared_ptr<RuleEngineInterface> evaluator) {
    m_evaluator = evaluator;
}

bool BatchAccumulator::checkBatchSize(size_t addedSize) const {
    return m_batchFile.getSize() + addedSize >= m_batchSize; 
}

size_t BatchAccumulator::getOverflowSize(size_t addedSize) const {    
    size_t totalSize = m_batchFile.getSize() + addedSize; // total number of objects I have 
    return (totalSize == m_batchSize) ? 0 : (totalSize - m_batchSize); 
}

void BatchAccumulator::accumulate(const TelemetryBatch &batch) {
    size_t size = batch.getSize();

    // it can be parallelized
    for(int i = 0; i < size; i++) {
        std::string sensors_name = batch.sensors_name[i];
        int64_t        timestamp = batch.timestamps[i];
        double             value = batch.values[i];
        int             priority = batch.priorities[i];
        m_batchFile.emplaceBack(sensors_name, timestamp, value, priority);
    }
}

void BatchAccumulator::storeValidData(TelemetryBatch &validBatch) {
    
    // check the batch size if not reach the specified m_batchSize then just accumulate;
    if(!checkBatchSize) {
        accumulate(validBatch);
        return;
    }

    // Prepare a cache in case of overflow values
    TelemetryBatch cacheBatch; // non abbiamo già m_batchTmp nella classe ??? 

    // Notice that if overflow = 0 then we do not enter into any loops
    size_t overflowSize = getOverflowSize(validBatch.getSize());

    // clear the cache and save a copy of excessed data into the cache
    for(int i = 0; i < overflowSize; i++) {
        std::string sensors_name = validBatch.sensors_name[i];
        int64_t        timestamp = validBatch.timestamps[i];
        double             value = validBatch.values[i];
        int             priority = validBatch.priorities[i];
        cacheBatch.emplaceBack(sensors_name, timestamp, value, priority);
    }

    // start the evaluation 
    //m_evaluator.evaluation(); 

    // if we use the thread safe buffer no need to call this here or no ???
    // please comment more

    // safely clear the current batch file
    m_batchFile.clear(); 

    //reload the values in the cache into the m_batchFile before starting to store new data
    accumulate(cacheBatch);

}

void BatchAccumulator::storeResultHistory() {
    
}
#include "BatchAccumulator.h"

size_t BatchAccumulator::getBatchSize() const {
    return m_batchSize;
}

const TelemetryBatch& BatchAccumulator::getBatchFile() const {
    return m_batchFile;
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
        // accumulate the single meausurement
        m_batchFile.emplaceBack(sensors_name, timestamp, value, priority);
        // send the single measuruement to the database
        m_database.storeResult(sensors_name, value); 
    }
}

void BatchAccumulator::storeValidData(TelemetryBatch &validBatch) {
    
    size_t validSize = validBatch.getSize();

    // check the batch size if not reach the specified m_batchSize then just accumulate;
    if(!checkBatchSize(validSize)) {
        accumulate(validBatch);
        return;
    }

    // Prepare a cache in case of overflow values
    TelemetryBatch cacheBatch; // non abbiamo già m_batchTmp nella classe ??? [Mike] Mi ero dimenticato di togliere m_batchTmp ma alla fine ha la stessa logica, solo diversa implementazione 

    // Notice that if overflow = 0 then we do not enter into any loops
    size_t overflowSize = getOverflowSize(validSize);

    // clear the cache and save a copy of excessed data into the cache
    for(int i = 0; i < overflowSize; i++) {
        std::string sensors_name = validBatch.sensors_name[i];
        int64_t        timestamp = validBatch.timestamps[i];
        double             value = validBatch.values[i];
        int             priority = validBatch.priorities[i];
        cacheBatch.emplaceBack(sensors_name, timestamp, value, priority);
    }

    // trigger the rule evaluation processing (Once included the queue it will eventually be commented)
    //m_evaluator.evaluateRules(); 

    // Producer Action
    m_broker.push(m_batchFile);

    // if we use the thread safe buffer no need to call this here or no ???
    // [Mike: I didn't understand the question, but at the moment do not care about the queue, 
    // the queue implementation will be included after the serial prototype of the software]  
    // please comment more [Mike: yes sorry i will try to comment more]


    // Safely clear the current batch file now that it has been sent
    m_batchFile.clear(); 

    //reload the values in the cache into the m_batchFile before starting to store new data
    accumulate(cacheBatch);

}

void BatchAccumulator::storeResult(std::string id, double value) const {
    m_database.storeResult(id, value);
}
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

/** @brief take m_batchSize elements from the temporal Batch and store it in the 
 * m_batchFile.
 */
void BatchAccumulator::accumulate(int start) {
    // it can be parallelized
    for(int i = 0; i < m_batchSize; i++) {
        int idx = i + start; // consider the starting index of the  
        std::string sensors_name = m_batchTmp.sensors_name[idx];
        int64_t        timestamp = m_batchTmp.timestamps[idx];
        double             value = m_batchTmp.values[idx];
        int             priority = m_batchTmp.priorities[idx];
        // accumulate the single meausurement
        m_batchFile.emplaceBack(sensors_name, timestamp, value, priority);
        // send the single measuruement to the database
        // m_database.storeResult(sensors_name, value); 
    }
}

/** @brief Receives data from the Ingestor and stores in the Tmp batch.
 * Later it checks if the dimension of the temporal batch is enough and
 * eventually calls 'accumulate' to populate the m_batchFile which is 
 * sent to the ThreadSafeBuffer queue.
 */
void BatchAccumulator::storeValidData(TelemetryBatch &validBatch) {
        
    size_t validSize = validBatch.getSize();

    // Instead of appending new data directly to the batchFile we first store it in the m_batcTmp
    for (size_t i = 0; i < validSize; ++i) {
        const std::string& sensors_name = validBatch.sensors_name[i];
        int64_t timestamp = validBatch.timestamps[i];
        double value = validBatch.values[i];
        int priority = validBatch.priorities[i];

        m_batchTmp.emplaceBack(sensors_name, timestamp, value, priority);
    }

    size_t tmpSize = m_batchTmp.getSize();
    if (tmpSize < m_batchSize) {
        return; // Not enough data to emit a full batch.
    }

    // Sends more than one batch if the offset is bigger than the m_batchFile
    size_t offset = 0;
    while (tmpSize - offset >= m_batchSize) {
        m_batchFile.clear();
        accumulate(offset); // start from the offset as long as we know there is at least offset elements in the batchTmp

        m_broker.push(m_batchFile);
        offset += m_batchSize;
    }

    // Keep only the remaining data in the cache.

    if ((tmpSize - offset) == 0) {
        m_batchTmp.clear(); // clear the batchTmp and return
        return;
    }

    // move the remaining objects in a dummy object and then move it to the start of m_batchTmp
    TelemetryBatch remaining(tmpSize - offset);
    for (size_t i = offset; i < tmpSize; ++i) {
        remaining.emplaceBack(
            m_batchTmp.sensors_name[i],
            m_batchTmp.timestamps[i],
            m_batchTmp.values[i],
            m_batchTmp.priorities[i]
        );
    }
    m_batchTmp = std::move(remaining);

    //reload the values in the cache into the m_batchFile before starting to store new data
    // accumulate(m_batchTmp);
}


// logic of this method need to be rewritten, since we need a database just for the stateful rules
// there is no need to store all the results but just the one associated to the rules that require it.
/*
void BatchAccumulator::storeResult(std::string id, double value) const {
    m_database.storeResult(id, value);
}
*/
#include "BatchAccumulator.h"

size_t BatchAccumulator::getBatchSize() const { return m_batchSize; }

const TelemetryBatch &BatchAccumulator::getBatchFile() const {
    return m_batchFile;
}

void BatchAccumulator::storeValidData(TelemetryBatch &validBatch) {

    size_t validSize = validBatch.getSize();

    for (int i = 0; i < validSize; i++) {

        // Prepare the paramters
        std::string sensors_name = validBatch.sensors_name[i];
        int64_t timestamp = validBatch.timestamps[i];
        double value = validBatch.values[i];
        int priority = validBatch.priorities[i];

        // Accumulate the single measure to the batch file
        m_batchFile.emplaceBack(sensors_name, timestamp, value, priority);

        // Compare the new size of the batch file with the threshold value for
        // the rule process triggeration
        if (m_batchFile.getSize() >= m_batchSize) {
            // Send a copy to the full batch file to the queue and once finished
            // clear safely the batch
            m_broker.push(m_batchFile);
            m_batchFile.clear();
        }
    }
}

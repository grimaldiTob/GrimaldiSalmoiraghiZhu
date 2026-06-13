#pragma once

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../interfaces/BatchAccumulatorInterface.h"
#include "../interfaces/ProducerBuffer.h"
#include "../types/TelemetryBatch.h"

/**
 * @brief Accumulates valid packets into a local batch for processing.
 * Implements BatchAccumulatorInterface for ingestion and BatchProviderInterface
 * for retrieval.
 */
class BatchAccumulator : public BatchAccumulatorInterface {

  public:
    /** @brief Constructor */
    explicit BatchAccumulator(ProducerBuffer<TelemetryBatch> &broker,
                              size_t batchSize = 100)
        : m_broker(broker), m_batchSize(batchSize) {}

    /*================= GETTER ==============*/
    size_t getBatchSize() const;
    const TelemetryBatch &getBatchFile() const;
    const TelemetryBatch &getBatchTmp() const;

    /*========================*/

    /** @brief Store a validated telemetry batch into the accumulator.
     *
     * This method accepts a batch of telemetry data that has already been
     * validated by the DataIngestor and accumulates it internally. Once the
     * batch reaches the configured size threshold, the accumulated batch
     * becomes available for processing by the rule engine.
     */
    void storeValidData(TelemetryBatch &validBatch) override;

  private:
    /* ============================= ATTRIBUTE ===========*/
    size_t m_batchSize;         // used to check wheter TelemetryBatch
    TelemetryBatch m_batchFile; // the current batch
    ProducerBuffer<TelemetryBatch>
        &m_broker; // buffer/queue to store batches that ary ready to be
                   // processed (will be uncomment once included the queue)
};

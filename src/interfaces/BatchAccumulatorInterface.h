#pragma once

#include "../types/TelemetryBatch.h"


/**
 * @brief validated telematry packets one by one
 */
class BatchAccumulatorInterface {

    virtual ~BatchAccumulatorInterface() = default;

    // ok so hypotetically this method receives a telemetry batch, which was populated by the ingestor
    virtual void storeValidData(TelemetryBatch&) = 0;

    virtual void sortPriorities() = 0;
};
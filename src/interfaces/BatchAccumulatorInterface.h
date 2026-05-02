#pragma once

#include "../types/TelemetryBatch.h"


/**
 * @brief validated telematry packets one by one
 */
class BatchAccumulatorInterface {

public:

    virtual ~BatchAccumulatorInterface() = default;

    // ok so hypotetically this method receives a telemetry batch, which was populated by the ingestor
    // it is more efficient to use references so that we can just erase the content of the batch and use it again.
    virtual void storeValidData(TelemetryBatch&) = 0;

    virtual void sortPriorities() = 0;
};
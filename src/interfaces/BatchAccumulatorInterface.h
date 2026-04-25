#pragma once

#include "../types/TelemetryBatch.h"


/**
 * @brief validated telematry packets one by one
 */
class BatchAccumulatorInterface {

    virtual ~BatchAccumulatorInterface() = default;

    virtual void sendValidData(TelemetryBatch) = 0;

};
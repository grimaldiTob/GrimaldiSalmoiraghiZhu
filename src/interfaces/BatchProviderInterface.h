#pragma once

#include<memory>
#include "../types/TelemetryBatch.h"

/**
 * @brief Provide access to the local batch file once it is ready for processing
 */

class BatchProviderInterface {

    virtual ~BatchProviderInterface() = default;

    virtual std::vector<TelemetryBatch> getBatchFile();

};
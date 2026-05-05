#pragma once

#include<memory>
#include "../types/TelemetryBatch.h"

/**
 * @brief Provide access to the local batch file once it is ready for processing
 */

class BatchProviderInterface {

public:

    virtual ~BatchProviderInterface() = default;

    virtual TelemetryBatch getBatchFile() const = 0;

};
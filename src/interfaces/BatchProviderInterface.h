#pragma once

#include "../types/TelemetryBatch.h"
#include <memory>

/**
 * @brief Provide access to the local batch file once it is ready for processing
 */

class BatchProviderInterface {

  public:
    virtual ~BatchProviderInterface() = default;

    virtual TelemetryBatch getBatchFile() const = 0;
};

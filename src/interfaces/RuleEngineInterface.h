#pragma once

#include "../types/TelemetryBatch.h"

class RuleEngineInterface {
public:
    virtual ~RuleEngineInterface() = default;

    virtual void evaluateRules(const TelemetryBatch& batch) = 0;

    virtual void resetCache() = 0;
};
#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <vector>
#include <memory>
#include "MeasDatabaseInterface.h"

class BaseRule;

class OutputDispatcherInterface {
public:
    virtual ~OutputDispatcherInterface() = default;

    virtual void appendValidData(const MeasDatabaseInterface&) = 0;

    virtual void appendAlarms(const MeasDatabaseInterface&, 
                     const std::vector<std::shared_ptr<BaseRule>>&) = 0;
};
#pragma once

#include "MeasDatabaseInterface.h"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class BaseRule;

class OutputDispatcherInterface {
  public:
    virtual ~OutputDispatcherInterface() = default;

    virtual void appendValidData(const MeasDatabaseInterface &,
                                 std::optional<int64_t>) = 0;

    virtual void appendAlarms(const MeasDatabaseInterface &,
                              const std::vector<std::shared_ptr<BaseRule>> &,
                              std::optional<int64_t>) = 0;
};

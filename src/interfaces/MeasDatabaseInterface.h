#pragma once

#include <vector>
#include <string>
#include <unordered_map>

/**
 * @brief validated telematry packets one by one
 */
class MeasDatabaseInterface {

public:

    virtual ~MeasDatabaseInterface() = default;

    virtual const std::unordered_map<std::string, std::vector<double>>& getMeasHistory() const = 0;

    virtual void storeResult(std::string, double) = 0;
    virtual void clearMeasurements(int n = 32) = 0;
};
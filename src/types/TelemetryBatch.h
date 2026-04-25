#pragma once

#include <vector>
#include <string>

struct TelemetryBatch {
    std::vector<std::string> sensors_name;
    std::vector<int64_t> timestamps;
    std::vector<double> values;
    std::vector<int> priorities;
};
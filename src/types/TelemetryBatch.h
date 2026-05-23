#pragma once

#include <string>
#include <vector>

struct TelemetryBatch {

    std::vector<std::string> sensors_name;
    std::vector<int64_t> timestamps;
    std::vector<double> values;
    std::vector<int> priorities;

    explicit TelemetryBatch(size_t reserveSize = 0) {
        if (reserveSize) {
            sensors_name.reserve(reserveSize);
            timestamps.reserve(reserveSize);
            values.reserve(reserveSize);
            priorities.reserve(reserveSize);
        }
    }

    size_t getSize() const { return sensors_name.size(); }

    void clear() {
        sensors_name.clear();
        timestamps.clear();
        values.clear();
        priorities.clear();
    }

    void emplaceBack(std::string id, int64_t timestamp, double value,
                     int priority) {
        sensors_name.emplace_back(std::string(id));
        timestamps.emplace_back(timestamp);
        values.emplace_back(value);
        priorities.emplace_back(priority);
    }
};

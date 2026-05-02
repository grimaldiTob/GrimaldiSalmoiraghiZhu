#pragma once

#include <vector>
#include <string>

struct TelemetryBatch {

    std::vector<std::string> sensors_name;
    std::vector<int64_t>       timestamps;
    std::vector<double>            values;
    std::vector<int>           priorities;
    int capacity;

    explicit TelemetryBatch(size_t reserveSize = 0) {
        if (reserveSize) {
            sensors_name.reserve(reserveSize);
            timestamps.reserve(reserveSize);
            values.reserve(reserveSize);
            priorities.reserve(reserveSize);
            capacity = reserveSize;
        }
    }

    void clear() {
        sensors_name.clear();
        timestamps.clear();
        values.clear();
        priorities.clear();
    }

    bool addMeausurement(std::string id, int64_t timestamp, double value, int priority) {
        if(sensors_name.size()) {
            
        }
    }



};
// Ingestor.h
#pragma once // Prevents the header from being included multiple times

#include <string>
#include <vector>
#include <cstdint>
#include "../external/simdjson/simdjson.h"

/*
    In Telemetry Batch we store the valid packets after filtering.
    I'm not quite sure we actually need a `timestamp` vector.
    For sure we need something that tells us how many valid packets
    were registered in a timestamp.
*/
struct TelemetryBatch {
    std::vector<std::string> sensors_name;
    std::vector<int64_t> timestamps;
    std::vector<double> values;
    std::vector<int> priorities;
};

void printTelemetry(const TelemetryBatch &batch, int limit);

int64_t parseISO8601(std::string_view time_str);

void parseTelemetry(simdjson::ondemand::parser &parser, std::string &filename, TelemetryBatch &valid_batch);

#include "DataIngestor.h"
#include <fstream>

// == Helpers
// ===================================================================

int DataIngestor::parsePriority(std::string_view priority_str) const {
    if (priority_str == "HIGH")
        return 2;
    if (priority_str == "MEDIUM")
        return 1;
    if (priority_str == "LOW")
        return 0;
    return -1; // unrecognised value
}

int64_t DataIngestor::parseISO8601(std::string_view time_str) {
    struct tm tm_struct = {};
    std::string s(time_str);
    // Expected format: YYYY-MM-DDTHH:MM:SSZ
    if (strptime(s.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm_struct) != nullptr)
        return timegm(&tm_struct);
    return 0; // return 0 if parsing fails
}

// == Debug printer
// ===============================================================

void DataIngestor::printTelemetry(const TelemetryBatch &batch,
                                  int limit) const {
    int records = static_cast<int>(batch.sensors_name.size());
    if (records < limit)
        limit = records;

    std::cout << "\n=== Telemetry Batch Preview (first " << limit << " of "
              << records << " records) ===\n";
    std::cout << std::left << std::setw(20) << "Sensor ID" << std::setw(20)
              << "Timestamp" << std::setw(20) << "Value" << std::setw(15)
              << "Priority" << "\n";
    std::cout << std::string(75, '-') << "\n";

    for (int i = 0; i < limit; ++i) {
        std::cout << std::left << std::setw(20) << batch.sensors_name[i]
                  << std::setw(20) << batch.timestamps[i] << std::setw(20)
                  << batch.values[i] << std::setw(15) << batch.priorities[i]
                  << "\n";
    }
    std::cout << std::string(75, '-') << "\n";
}

// ================================================================================0

void DataIngestor::sendValidBatchToAccumulator() {
    m_accumulator.storeValidData(m_validBatch);
}

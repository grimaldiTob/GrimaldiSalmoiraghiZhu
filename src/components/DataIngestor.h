#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "../../external/simdjson.h"
#include "../types/TelemetryBatch.h"
#include "../interfaces/BatchAccumulatorInterface.h"

class DataIngestor { 

public:

    DataIngestor() = default;

    void printTelemetry(const TelemetryBatch &batch, int limit);

    int64_t parseISO8601(std::string_view time_str);

    void parseTelemetry(simdjson::ondemand::parser &parser, const std::string &filename);
    
    void setAccumulatorInterface(std::shared_ptr<BatchAccumulatorInterface> accumulatorInterface);

private:

    std::shared_ptr<BatchAccumulatorInterface> m_accumulatorInterface;
    TelemetryBatch m_validBatch; // this batch contains valid data and will be send it to the accumulator

    void sendValidBatchToAccumulator();
    
}; 
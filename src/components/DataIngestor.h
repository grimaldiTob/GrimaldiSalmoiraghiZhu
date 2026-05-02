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

    void parseTelemetry(simdjson::ondemand::parser &parser, 
                        const std::string        &filename, 
                        TelemetryBatch        &valid_batch);
    
    void setAccumulatorInterface(BatchAccumulatorInterface& accumulatorInterface);

private:
    
    std::shared_ptr<BatchAccumulatorInterface> m_accumulatorInterface;

}; 
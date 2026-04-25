#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "../interfaces/InvalidPacketsFilterInterface.h"
#include "../../external/simdjson.h"
#include "../types/TelemetryBatch.h"

class DataIngestor { 

public:

    DataIngestor(std::shared_ptr<InvalidPacketsFilterInterface> filter);

    void printTelemetry(const TelemetryBatch &batch, int limit);

    int64_t parseISO8601(std::string_view time_str);

    void parseTelemetry(simdjson::ondemand::parser &parser, 
                        const std::string &filename, 
                        TelemetryBatch &valid_batch);

private:

    std::shared_ptr<InvalidPacketsFilterInterface> m_filter;
    
}; 
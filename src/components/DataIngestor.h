#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "../../external/simdjson.h"
#include "../types/TelemetryBatch.h"
#include "../interfaces/BatchAccumulatorInterface.h"

/**
 * @brief Main functionality of the script is to read content of JSON packets
 * collected in ./src/collector_output, filtering invalid packets and
 * storing the valid ones in a `TelemetryBatch` structure.
 */
class DataIngestor { 

public:

    DataIngestor() = default;

    /** 
     * @brief Telemetry Printer
     * 
     * Used for debugging for the implementation of parseTelemetry()
     */
    void printTelemetry(const TelemetryBatch &batch, int limit) const; 

    /** 
     * @brief Parsering method
     * 
     * Parses the file that contains a batch of JSON packets received from the spacecraft.
     * Stores the valid data in a `m_validBatch` structure and filters out invalid packets.
     */
    void parseTelemetry(simdjson::ondemand::parser &parser, const std::string &filename);

    /** 
     * @brief  TODO
     */ 
    void setAccumulatorInterface(std::shared_ptr<BatchAccumulatorInterface> accumulatorInterface);
    
private:
        
    /** 
     * @brief Converstion string_view -> int64_7 
     * 
     * Converts the timestamp present in the collector output files 
     * in an integer at epoch time.
     */
    int64_t parseISO8601(std::string_view time_str);

    void    sendValidBatchToAccumulator();
    
    std::shared_ptr<BatchAccumulatorInterface> m_accumulatorInterface;
    TelemetryBatch                             m_validBatch; // this batch contains valid data and will be send it to the accumulator

}; 
#include<memory>

#include "../interfaces/BatchAccumulatorInterface.h"
#include "../interfaces/InvalidPacketsFilterInterface.h"
#include "../types/TelemetryBatch.h"

class InvalidPacketsFilter : public InvalidPacketsFilterInterface {

private:

    std::shared_ptr<BatchAccumulatorInterface> m_accumulator;
    
    // Help to transform a valid JSON string into a TelematryBatch object
    TelemetryBatch createTelematryBatch(const std::string&); 

public:
 
    InvalidPacketsFilter(std::shared_ptr<BatchAccumulatorInterface> accumulator);

    void sendRawData(const std::string& rawJson) override; 

};
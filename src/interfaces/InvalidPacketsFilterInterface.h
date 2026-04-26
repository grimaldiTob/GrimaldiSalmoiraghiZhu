#pragma once
#include <string>

// class to discuss
class InvalidPacketsFilterInterface {

public:

    virtual ~InvalidPacketsFilterInterface() = default;

    /**
     * @brief Receive and evaluate incoming raw JSON data for validity
     * and, if valid, then build the corresponding TelematryBatch object 
     * which will be dispatched to the Accumulator class 
     */
    virtual void sendRawData(const std::string& rawJson) = 0;

};
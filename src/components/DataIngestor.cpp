#include "DataIngestor.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

void DataIngestor::printTelemetry(const TelemetryBatch &batch,
                                  int limit = 10) const {
    int records = batch.sensors_name.size();
    if (records < limit) {
        limit = records;
    }

    std::cout
        << "\n=== Telemetry Batch Preview (First limit total records) ===\n";
    std::cout << std::left << std::setw(20) << "Sensor ID" << std::setw(20)
              << "Timestamp" << std::setw(20) << "Value" << std::setw(15)
              << "Priority" << "\n";
    std::cout << std::string(70, '-') << "\n";

    for (size_t i = 0; i < limit; ++i) {
        std::cout << std::left << std::setw(20) << batch.sensors_name[i]
                  << std::setw(20) << batch.timestamps[i] << std::setw(20)
                  << batch.values[i] << std::setw(15) << batch.priorities[i]
                  << "\n";
    }
    std::cout << std::string(70, '-') << "\n";
}

int64_t DataIngestor::parseISO8601(std::string_view time_str) {
    struct tm tm_struct = {0};
    std::string s(time_str);

    // parse the format YYYY-MM-DDTHH:MM:SSZ
    if (strptime(s.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm_struct) != nullptr) {
        return timegm(&tm_struct); // Convert to Unix epoch
    }
    return 0; // Return 0 if parsing fails
}

void DataIngestor::parseTelemetry(simdjson::ondemand::parser &parser,
                                  const std::string &filename) {

    // remove the content from the previous file
    m_validBatch.clear();

    int valid_pkg = 0;
    int invalid_pkg = 0;

    // Read iteratively of record NDJSON
    simdjson::padded_string fileStream;
    auto error = simdjson::padded_string::load(filename).get(fileStream);

    // Throw an exception containing the exact simdjson error message
    if (error)
        throw std::runtime_error("SIMDJSON ERROR: " +
                                 std::string(simdjson::error_message(error)));

    // Throw an error in case of reading an empty file
    if (fileStream.size() == 0)
        throw std::runtime_error("DataIngestor Error: The file exists, but it "
                                 "is completely empty (0 bytes)!");

    std::ifstream infile(filename); // just one file at a time for now
    std::string line;

    // get_line takes a ifstream in input and a string and stores the content in
    // the string until it finds the "\n" char
    while (std::getline(infile, line)) {
        if (line.empty())
            continue;

        simdjson::padded_string padded_line(line); // create a new padded line
        simdjson::ondemand::document doc;

        auto error = parser.iterate(padded_line)
                         .get(doc); // .get() method assigns the value to the
                                    // argument passed to the function
        if (error) {
            invalid_pkg++;
            std::cerr << "Impossible JSON packet syntax" << std::endl;
            continue; // skip to the next line here
        }

        std::string_view sensor_id;
        std::string_view timestamp;
        double value;
        std::string_view priority_str; // supported type by simdjson

        // check if all the values are present and of a valid type
        if (doc["timestamp"].get(timestamp) ==
                simdjson::SUCCESS && // return true if the assignment made by
                                     // .get() does not generate errors.
            doc["sensor_id"].get(sensor_id) ==
                simdjson::SUCCESS && // if a value cannot be assigned to a
                                     // variable it returns an error --> false
            doc["value"].get(value) == simdjson::SUCCESS) {
            int64_t timestamp_epoch = parseISO8601(timestamp);
            int priority = 0; // default value set to 0

            // priority value handling
            if (doc["priority"].get(priority_str) == simdjson::SUCCESS) {
                if (priority_str == "HIGH") {
                    priority = 2;
                } else if (priority_str == "MEDIUM") {
                    priority = 1;
                } else if (priority_str == "LOW") {
                    priority = 0;
                } else // IF the priority value is present and it is a string,
                       // checks if the value is coherent
                {
                    // filter the package even if the priority value is wrong
                    // ???
                    invalid_pkg++;
                    continue;
                }
            }
            // check the string validity ??? I
            m_validBatch.emplaceBack(std::string(sensor_id), timestamp_epoch,
                                     value, priority);
            valid_pkg++;
        } else {
            invalid_pkg++; // at least one value type was not correct
            continue;
        }
    }
    std::cout << "Valid packages: " << valid_pkg << std::endl;
    std::cout << "Invalid packages: " << invalid_pkg << std::endl;
    printTelemetry(m_validBatch);

    // Finally send valid package
    sendValidBatchToAccumulator();
}

void DataIngestor::sendValidBatchToAccumulator() {
    m_accumulator.storeValidData(m_validBatch);
}

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../external/simdjson/simdjson.h"

/*
    Basic first implementation of an Ingestor Component!

    Main functionality of the script is to read content of JSON packets
    collected in ./src/collector_output, filtering invalid packets and
    storing the valid ones in a `TelemetryBatch` structure. 
*/

/*
    In Telemetry Batch we store the valid packets after filtering.
    I'm not quite sure we actually need a `timestamp` vector.
    For sure we need something that tells us how many valid packets
    were registered in a timestamp.
*/
struct TelemetryBatch {
    std::vector<std::string> sensors_name;
    std::vector<int64_t> timestamps; // parsing of timestamps from strings to integer required
    std::vector<double> values; // array of values
};

void printTelemetry(const TelemetryBatch &batch, int limit = 10) {
    int records = batch.sensors_name.size();
    if(records < limit) {
        limit = records;
    }

    std::cout << "\n=== Telemetry Batch Preview (First limit total records) ===\n";
    std::cout << std::left 
              << std::setw(20) << "Sensor ID" 
              << std::setw(20) << "Timestamp" 
              << std::setw(15) << "Value" << "\n";
    std::cout << std::string(55, '-') << "\n";

    for (size_t i = 0; i < limit; ++i) {
        std::cout << std::left 
                  << std::setw(20) << batch.sensors_name[i]
                  << std::setw(20) << batch.timestamps[i]
                  << std::setw(15) << batch.values[i] << "\n";
    }
    std::cout << std::string(55, '-') << "\n";
}

int main(void) {
    // initialize the simdjson parser --> in short: simdjson parses multiple json data in parallel, being memory efficient aswell.
    simdjson::ondemand::parser parser;
    TelemetryBatch valid_batch;

    int valid_pkg = 0;
    int invalid_pkg = 0;

    std::ifstream infile("./collector_output/raw_data_1776874413_0000.txt"); // just one file at a time for now
    std::string line;

    // get_line takes a ifstream in input and a string and stores the content in the string until it finds the "\n" char 
    while(std::getline(infile, line)) {
        if(line.empty()) continue;

        simdjson::padded_string padded_line(line); // create a new padded line
        simdjson::ondemand::document doc;

        auto error = parser.iterate(padded_line).get(doc); // .get() method assigns the value to the argument passed to the function 
        if(error) {
            invalid_pkg++;
            std::cerr << "Impossible JSON packet syntax" << std::endl;
            continue; // skip to the next line here
        }

        std::string sensor_id;
        int64_t timestamp;
        double value;

        // check if all the values are of a valid type
        if (doc["sensor_id"].get(sensor_id) == simdjson::SUCCESS && // return true if the assignment made by .get() does not generate errors.
            doc["timestamp"].get(timestamp) == simdjson::SUCCESS && // if a value cannot be assigned to a variable it returns an error --> false
            doc["value"].get(value) == simdjson::SUCCESS) 
        {
            // check the string validity ??? 

            valid_batch.sensors_name.emplace_back(sensor_id);
            valid_batch.timestamps.emplace_back(timestamp);
            valid_batch.values.emplace_back(value);
            valid_pkg++;
        } else {
            invalid_pkg++; // at least one value type was not correct
            continue;
        }

    }

    return 0;
}

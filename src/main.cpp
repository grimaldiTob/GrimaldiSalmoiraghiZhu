#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <string>

#include "./components/DataIngestor.h"

/*
    My personal ideas on the batchAccumulator:
        - either we create an array of "Packets" ordered by Priority
        - either we use the same approach as TelemetryBatch with arrays of fixed dimension
        - add some pointers that sign the end of high-priority and medium-priority packets --> easy insert 
*/
struct batchAccumulator {
};

int main(void) {
    // initialize the simdjson parser --> in short: simdjson parses multiple json data in parallel, being memory efficient aswell.
    simdjson::ondemand::parser parser;
    TelemetryBatch shared_batch;
    DataIngestor ingestor = DataIngestor();
    const std::string output_dir = "../collector_output";

    int count = 0;

    while(true) {
        // checks wheter the directory is empty or not --> not sure if we need to implement a waiting timer in order to wait for new packets
        if(std::filesystem::is_empty(output_dir) || count == 1) { // added count check just to complete the loop
            std::cout << "Directory Empty." << "\n";
            break;
        }
        for (const auto &entry : std::filesystem::directory_iterator(output_dir))
        {
            if(entry.path().extension() == ".txt"){
                ingestor.parseTelemetry(parser, entry.path().string(), shared_batch);

                // TODO: we need the accumulator structure that evaluates packets in priority order.

                // std::filesystem::remove(entry.path());
            }
        }
        count++;
    }
    std::cout << "Finished..." << "\n";
    return 0;
}
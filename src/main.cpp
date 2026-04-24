#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <string>

#include "Ingestor.h"

int main(void) {
    simdjson::ondemand::parser parser;
    TelemetryBatch shared_batch;
    const std::string output_dir = "./collector_output";

    while(true) {

    }

    return 0;
}
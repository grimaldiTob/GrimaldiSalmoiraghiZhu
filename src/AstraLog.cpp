#include "AstraLog.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr size_t DEFAULT_BATCH_SIZE = 32;
constexpr size_t DEFAULT_QUEUE_SIZE = 32;
constexpr auto kPollInterval = std::chrono::milliseconds(
    500); // interval of polling in the ./collector_output
constexpr auto kIdleTimeout =
    std::chrono::seconds(5); // output of maximum idle time we can handle.
} // namespace

/** @brief Reads each single entry in the directory filename
 * passed as input and calls the method ParseTelemetry to initiate
 * the parsing procedure.
 */
void AstraLog::readInput(const std::string &filename) {
    std::filesystem::path inputPath(filename);

    // check in order to be fault tolerant.
    if (!std::filesystem::exists(inputPath)) {
        throw std::runtime_error("Input path does not exist: " + filename);
    }

    if (std::filesystem::is_directory(inputPath)) {
        // consider a vector of files
        std::vector<std::filesystem::path> files;
        for (const auto &entry :
             std::filesystem::directory_iterator(inputPath)) {
            // here we could handle the case in which we take the data from
            // csv_input
            if (entry.path().extension() == ".txt") {
                files.emplace_back(entry.path());
            }
        }

        for (const auto &file : files) {
            m_ingestor->parseTelemetry(file.string());
        }
        return;
    }
    // if filename is a file it calles ParseTelemetry as it is
    m_ingestor->parseTelemetry(inputPath.string());
}

/** @brief Main orchestrator of the program:
 *  ============= WHAT IT DOES? ==============
 *  Generates two threads:
 *      1) manages data ingestion
 *      2) manages Rule evaluation
 *  Both threads run a while loop. One waits for the batches to be accumulated
 * on the buffer, the other one checks periodically the output directory.
 *
 *  In the end the main thread checks wheter both the queue and the directory
 * are empty. If they are for a certain interval it stops the execution of loops
 * and joins threads.
 */
void AstraLog::run(const std::string &inputPath, const std::string &rulesPath) {

    // check if file exist here since I use try-catch block in the main method.
    std::filesystem::path inputDir(inputPath);
    if (!std::filesystem::is_directory(inputDir)) {
        throw std::runtime_error("Input path is not a directory: " + inputPath);
    }

    std::filesystem::path rulesFile(rulesPath);
    if (!std::filesystem::exists(rulesFile)) {
        throw std::runtime_error("Rules path is not a file: " + rulesPath);
    }

    m_evaluator->setRulesFilename(rulesPath);
    m_evaluator->setRulesList(
        *m_loader); // first of all load rules inside the RuleEngine

    std::atomic<bool> stopRequested{
        false}; // concurrent operations on std::atomic are "well-defined"
    // which basically means they do not incur in race conditons.

    // run in one thread the run loop of the RuleEngine
    std::thread evaluatorThread([this]() { m_evaluator->run(); });

    std::thread ingestorThread([this, &inputDir, &stopRequested]() {
        while (!stopRequested.load()) {
            bool foundFile = false;

            for (const auto &entry :
                 std::filesystem::directory_iterator(inputDir)) {
                if (entry.path().extension() != ".txt") {
                    continue;
                } else {
                    foundFile = true;

                    // since parseTelemetry method throws some exceptions we
                    // need to handle them
                    try {
                        m_ingestor->parseTelemetry(entry.path().string());
                        std::filesystem::remove(entry.path());
                        // APPROACH: erase files that have been parsed --> good
                        // for storage, not so good for availability and
                        // resilience of the data. Should we consider to store
                        // them in another folder?
                    } catch (const std::exception &ex) {
                        std::cerr << "Ingestor error: " << ex.what() << '\n';
                    }
                }
            }

            if (!foundFile) {
                std::this_thread::sleep_for(kPollInterval);
            }
        }
    });

    // the main thread checks if the system is in idle or not.
    auto idleStart = std::chrono::steady_clock::now();
    while (true) {
        const bool dirEmpty = std::filesystem::is_empty(inputDir);
        const bool queueEmpty = m_broker->getQueue().empty();

        // if both the collector directory and the queue are empty check the
        // system idle state
        if (dirEmpty && queueEmpty) {
            if (std::chrono::steady_clock::now() - idleStart >= kIdleTimeout) {
                stopRequested.store(true); // set to true so that the ingestor
                                           // is not collecting data anymore
                m_broker->finish_production();
                break;
            }
        } else {
            idleStart = std::chrono::steady_clock::now();
        }

        std::this_thread::sleep_for(
            kPollInterval); // makes this thread sleep for an Interval
    }

    if (ingestorThread.joinable()) {
        ingestorThread.join();
    }
    if (evaluatorThread.joinable()) {
        evaluatorThread.join();
    }
}

// HYPOTHETICAL ENTRY POINT OF OUR APPLICATION
#ifndef ASTRALOG_NO_MAIN
int main(int argc, char **argv) {
    std::string inputPath = "./collector_output";
    std::string rulesPath = "./input/rules.json";
    size_t batchSize = DEFAULT_BATCH_SIZE;
    size_t queueSize = DEFAULT_QUEUE_SIZE;

    // basic parsing considering to pass different batch and queue size, --> I
    // dont think we need Bucelli's library here
    if (argc > 1) {
        inputPath = argv[1];
    }
    if (argc > 2) {
        rulesPath = argv[2];
    }
    if (argc > 3) {
        batchSize = static_cast<size_t>(std::stoul(argv[3]));
    }
    if (argc > 4) {
        queueSize = static_cast<size_t>(std::stoul(argv[4]));
    }

    try {
        AstraLog astralog(batchSize, queueSize);
        astralog.run(inputPath, rulesPath);
    } catch (const std::exception &ex) {
        std::cerr << "AstraLog error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
#endif

#include "AstraLog.h"

// includes and constructor moved to .cpp file in order to keep .mpi in this
// file
#ifdef ASTRALOG_MPI
#include "components/MpiRuleEngine.h"
#include <mpi.h>
#endif

#include <atomic>
#include <chrono>
#include <clipp.h>
#include <filesystem>
#include <fstream>
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
    std::chrono::seconds(10); // output of maximum idle time we can handle.
} // namespace

static std::unique_ptr<RuleEngine>
makeRuleEngine(bool useMpi, ConsumerBuffer<TelemetryBatch> &broker,
               MeasDatabaseInterface &db, OutputDispatcherInterface &out,
               RuleLoaderInterface &loader, std::optional<int64_t> ts) {

// we can discuss on the macro approach --> basically for me macro is better
// because it allows to avoid completely mpi compilation if not needed.
#ifdef ASTRALOG_USE
    // we can set useMpi as a runtime or a constant time variable (MACRO???)
    if (useMpi) {
        return std::make_unique<MpiRuleEngine>(broker, db, out, loader, ts,
                                               MPI_COMM_WORLD);
    }
#endif
    return std::make_unique<RuleEngine>(broker, db, out, loader, ts);
}

static std::unique_ptr<DataIngestor>
makeDataIngestor(bool useCSV, BatchAccumulatorInterface &accumulator) {
    if (useCSV)
        return std::make_unique<CsvDataIngestor>(accumulator);
    else
        return std::make_unique<JsonDataIngestor>(accumulator);
}

AstraLog::AstraLog(bool useMpi, bool useCSV, size_t batchSize,
                   size_t queueSize) {
    m_database = std::make_unique<MeasDatabase>();
    m_broker = std::make_shared<ThreadSafeBuffer<TelemetryBatch>>(queueSize);
    m_accumulator = std::make_unique<BatchAccumulator>(*m_broker, batchSize);
    m_outputDispatcher = std::make_unique<OutputDispatcher>();
    m_loader = std::make_unique<RuleLoader>();
    m_ingestor = makeDataIngestor(useCSV, *m_accumulator);
    m_evaluator = makeRuleEngine(useMpi, *m_broker, *m_database,
                                 *m_outputDispatcher, *m_loader, std::nullopt);
}

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
    m_evaluator
        ->setRulesList(); // first of all load rules inside the RuleEngine

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
    bool useMpi = false;
    bool useCsv = false;
    bool showHelp = false;

#ifdef ASTRALOG_MPI
    MPI_Init(&argc, &argv);
    useMpi = true;
#endif

    using namespace clipp;

    auto cli =
        ((option("--input") & value("input_path", inputPath)) |
             opt_value("input_path", inputPath) %
                 "Input directory path (default ./collector_output)",
         (option("--rules") & value("rules_path", rulesPath)) |
             opt_value("rules_path", rulesPath) %
                 "Rules file path (default ./input/rules.json)",
         (option("--batch") & value("batch_size", batchSize)) |
             opt_value("batch_size", batchSize) % "Batch size (default 32)",
         (option("--queue") & value("queue_size", queueSize)) |
             opt_value("queue_size", queueSize) % "Queue size (default 32)",
         option("--csv").set(useCsv) % "Parse input files as CSV",
         option("--mpi").set(useMpi) % "Run using MPI-enabled rule engine",
         option("-h", "--help").set(showHelp) % "Show this help message");

    if (!parse(argc, argv, cli)) {
        std::cout << "Error on using commands.\n";
        std::cout << make_man_page(cli, argv[0]);
        return 1;
    }

    if (showHelp) {
        std::cout << make_man_page(cli, argv[0]);
        return 0;
    }

    try {
        // --- Output directory and file initialization ---
        std::string validDataPath = "./output/valid_data.csv";
        std::string alarmsPath = "./output/alarms.log";
        bool isRank0 = true; // handle file creation with OpenMPI.

#ifdef ASTRALOG_MPI
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        isRank0 = (rank == 0);
#endif

        if (isRank0) {
            namespace fs = std::filesystem;
            fs::create_directories(fs::path(validDataPath).parent_path());
            fs::create_directories(fs::path(alarmsPath).parent_path());

            // Initialize empty files if they don't exist
            std::ofstream(validDataPath, std::ios::app);
            std::ofstream(alarmsPath, std::ios::app);
        }

#ifdef ASTRALOG_MPI
        // wait for each rank
        MPI_Barrier(MPI_COMM_WORLD);
#endif
        AstraLog astralog(useMpi, useCsv, batchSize, queueSize);
        astralog.run(inputPath, rulesPath);

    } catch (const std::exception &ex) {
        std::cerr << "AstraLog error: " << ex.what() << '\n';
#ifdef ASTRALOG_MPI
        MPI_Finalize();
#endif
        return 1;
    }
#ifdef ASTRALOG_MPI
    MPI_Finalize();
#endif
    return 0;
}

#endif

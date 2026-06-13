#pragma once

#include "../external/simdjson.h"
#include "components/BatchAccumulator.h"
#include "components/MeasDatabase.h"
#include "components/OutputDispatcher.h"
#include "components/RuleEngine.h"
#include "components/RuleLoader.h"
#include "components/ThreadSafeBuffer.h"
#include "components/data_ingestor/CsvDataIngestor.h"
#include "components/data_ingestor/JsonDataIngestor.h"

#include "types/TelemetryBatch.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

class AstraLog {

  public:
    /**
     * @brief Constructor that instanciate every components of the system and
     * provide, for all, the necessary interfaces which allow them
     * to communicate
     */
    AstraLog(bool useMpi, bool useCSV, size_t batchSize = 100,
             size_t queueSize = 50);

    /*============== GETTER ====================*/
    /*
    Never implemented + never used
    const DataIngestor& getIngestor() const;
    const BatchAccumulator& getAccumulator() const;
    const RuleEngine& getEvaluator() const;
    */

    /** @brief read the input from the filename */
    void readInput(const std::string &filename);

    /** @brief run the full pipeline on a path (file or directory) */
    void run(const std::string &inputPath = "collector_output",
             const std::string &rulesPath = "./rules.json");

  private:
    bool shouldProcessFile(const std::filesystem::path &path) const;
    bool hasMatchingFiles(const std::filesystem::path &dir) const;

    std::unique_ptr<DataIngestor> m_ingestor;
    std::unique_ptr<BatchAccumulator> m_accumulator;
    std::unique_ptr<RuleEngine> m_evaluator;
    std::unique_ptr<RuleLoader> m_loader;
    std::unique_ptr<MeasDatabase> m_database;
    std::unique_ptr<OutputDispatcher> m_outputDispatcher;
    std::shared_ptr<ThreadSafeBuffer<TelemetryBatch>> m_broker;
    bool m_useCsv = false;
};

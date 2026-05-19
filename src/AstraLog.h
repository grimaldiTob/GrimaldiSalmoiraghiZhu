#include <memory>
#include <optional>
#include <string>
#include "types/TelemetryBatch.h"
#include "components/DataIngestor.h"
#include "components/BatchAccumulator.h"
#include "components/RuleEngine.h"
#include "components/RuleLoader.h"
#include "components/MeasDatabase.h"
#include "components/ThreadSafeBuffer.h"
#include "../../external/simdjson.h"


class AstraLog {

public:

    /**
     * @brief Constructor that instanciate every components of the system and 
     * provide, for all, the necessary interfaces which allow them 
     * to communicate 
     */
    AstraLog(size_t batchSize = 100, size_t queueSize = 50) {
        m_database = std::make_unique<MeasDatabase>();
        m_broker = std::make_shared<ThreadSafeBuffer<TelemetryBatch>>(queueSize);
        m_accumulator = std::make_unique<BatchAccumulator>(*m_broker, batchSize);
        m_ingestor = std::make_unique<DataIngestor>(*m_accumulator);
        m_evaluator = std::make_unique<RuleEngine>(*m_broker, *m_database, std::nullopt);
        m_loader = std::make_unique<RuleLoader>();
    }
    
    /*============== GETTER ====================*/
    const DataIngestor        getIngestor() const;
    const BatchAccumulator getAccumulator() const;
    const RuleEngine         getEvaluator() const;

    /** @brief read the input from the filename */
    void readInput(const std::string &filename);

    /** @brief run the full pipeline on a path (file or directory) */
    void run(const std::string &inputPath = "collector_output");

private:

    std::unique_ptr<DataIngestor>                     m_ingestor;
    std::unique_ptr<BatchAccumulator>                 m_accumulator;  
    std::unique_ptr<RuleEngine>                       m_evaluator;
    std::unique_ptr<RuleLoader>                       m_loader;
    std::unique_ptr<MeasDatabase>                     m_database;
    std::shared_ptr<ThreadSafeBuffer<TelemetryBatch>> m_broker;
};
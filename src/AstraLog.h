#include <memory>
#include "types/TelemetryBatch.h"
#include "components/DataIngestor.h"
#include "components/BatchAccumulator.h"
#include "components/RuleEngine.h"
#include "components/RuleLoader.h"
#include "components/MeasDatabase.h"
#include "../../external/simdjson.h"


class AstroLog {

public:

    /**
     * @brief Constructor that instanciate every components of the system and 
     * provide, for all, the necessary interfaces which allow them 
     * to communicate 
     */
    AstroLog(int batchSize = 100) {
        m_database = std::make_unique<MeasDatabase>();
        m_ingestor = std::make_unique<DataIngestor>();
        m_accumulator = std::make_unique<BatchAccumulator>(*m_evaluator, *m_database, batchSize);
        m_evaluator = std::make_unique<RuleEngine>(); // TODO: make a constructor that instanciate every interfaces used by this class
        m_loader = std::make_unique<RuleLoader>();    // TODO: make a constructor that instaciate every interfaces used by this class
    }
    
    /*============== GETTER ====================*/
    const DataIngestor        getIngestor() const;
    const BatchAccumulator getAccumulator() const;
    const RuleEngine         getEvaluator() const;

    /** @brief read the input from the filename */
    void readInput(const std::string &filename);

private:

    std::unique_ptr<DataIngestor>          m_ingestor;
    std::unique_ptr<BatchAccumulator>      m_accumulator;  
    std::unique_ptr<RuleEngine>            m_evaluator;
    std::unique_ptr<RuleLoader>            m_loader;
    std::unique_ptr<MeasDatabase>          m_database;

};
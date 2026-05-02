#include "types/TelemetryBatch.h"
#include "components/DataIngestor.h"
#include "components/BatchAccumulator.h"
#include "components/RuleEngine.h"
#include <memory>

class AstroLog {

public:

    /**
     * @brief Constructor that instanciate every components of the system and 
     * provide, for all, the necessary interfaces which allow them 
     * to communicate 
     */
    AstroLog(int batchSize = 100)
        : m_accumulator(std::make_shared<BatchAccumulator>(batchSize)),
          m_ingestor(std::shared_ptr<DataIngestor>()),
          m_evaluator(std::shared_ptr<RuleEngine>()) {
            m_ingestor->setAccumulatorInterface(m_accumulator);
            m_accumulator->setRuleEngineInterface(m_evaluator);
            // TODO: implement RuleLoader and OutputDispatcher classes
          }
    
    // Might be usefull for debugging
    DataIngestor        getIngestor() const;
    BatchAccumulator getAccumulator() const;
    RuleEngine         getEvaluator() const;

private:

    std::shared_ptr<DataIngestor>           m_ingestor;
    std::shared_ptr<BatchAccumulator>    m_accumulator;
    std::shared_ptr<RuleEngine>            m_evaluator;
    //RuleLoader              m_loader;
    //OutputDispacther    m_dispatcher;
    //DataBase for stateful rule

};
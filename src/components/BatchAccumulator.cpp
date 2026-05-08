#include "BatchAccumulator.h"

size_t BatchAccumulator::getBatchSize() const {
    return m_batchSize;
}

// modified the method so it is not returning by value the whole batch 
const TelemetryBatch& BatchAccumulator::getBatchFile() const {
    return m_batchFile;
}

void BatchAccumulator::setEvaluator(std::shared_ptr<RuleEngineInterface> evaluator) {
    m_evaluator = evaluator;
}

size_t BatchAccumulator::checkBatchSize(size_t addedSize) const {
    return m_batchFile.getSize() + addedSize - m_batchSize; 
}

void BatchAccumulator::storeValidData(TelemetryBatch &validBatch) {
    
    size_t availableSpace = checkBatchSize(validBatch.getSize());

    // CASE: 
    if(availableSpace > 0) {
        m_batchFile.append(validBatch);
        return;
    }

    handleOverUploading();

}

// non capisco Mike
class A {

    void methodA();

    void methodB();
};
#include "BatchAccumulator.h"

size_t BatchAccumulator::getBatchSize() { return m_batchSize; }

TelemetryBatch BatchAccumulator::getBatchFile() { return m_batchFile;  }

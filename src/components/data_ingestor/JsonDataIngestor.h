#pragma once
#include "DataIngestor.h"

class JsonDataIngestor : public DataIngestor {
  public:
    JsonDataIngestor(BatchAccumulatorInterface &accumulator)
        : DataIngestor(accumulator) {}

    void parseTelemetry(const std::string &filename) override;

  private:
    simdjson::ondemand::parser m_parser;
};

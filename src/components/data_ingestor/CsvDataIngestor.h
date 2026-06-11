#pragma once
#include "DataIngestor.h"

class CsvDataIngestor : public DataIngestor {

  public:
    CsvDataIngestor(BatchAccumulatorInterface &accumulalotor)
        : DataIngestor(accumulalotor) {}

    void parseTelemetry(const std::string &filename) override;

  private:
    std::vector<std::string> splitCSVRow(const std::string &line) const;
};

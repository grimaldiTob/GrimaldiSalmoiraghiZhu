#pragma once
#include "../types/TelemetryBatch.h"
#include <memory>
#include <string>
#include <vector>

// I dont get the usage of this class ???
// I see it is not used anywhere
class BatchFile {

  public:
    BatchFile() = default;

    void addPacket(const TelemetryBatch &packet);

    void clear();

    bool isEmpty() const;

  private:
    std::vector<TelemetryBatch> packets;
};

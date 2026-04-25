#pragma once
#include <vector>
#include <string>
#include <memory>
#include "../types/TelemetryBatch.h"

class BatchFile {

public:

    BatchFile() = default;

    void addPacket(const TelemetryBatch& packet);

    void clear();

    bool isEmpty() const;

private:

    std::vector<TelemetryBatch> packets;

};
#pragma once
#include <vector>
#include <string>
#include <memory>
#include "../types/TelemetryBatch.h"

// I dont get the usage of this class ???
// I see it is not used anywhere
class BatchFile {

public:
    BatchFile() = default;

    void addPacket(const TelemetryBatch& packet);

    void clear();

    bool isEmpty() const;

private:

    std::vector<TelemetryBatch> packets;

};
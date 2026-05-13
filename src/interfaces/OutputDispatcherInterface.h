#include <string>
#include <optional>
#include <unordered_map>

class OutputDispatcherInterface {
public:
    virtual ~OutputDispatcherInterface() = default;

    virtual void appendValidData(const TelemetryBatch&) = 0;

    virtual void appendAlarms(const TelemetryBatch&, 
                     const std::unordered_map<std::string, std::optional<bool>>&) = 0;
};
#pragma once

#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include "../interfaces/OutputDispatcherInterface.h"
#include "../types/rules/BaseRule.h"

// Forward declarations to reduce header coupling
class MeasDatabaseInterface;

class OutputDispatcher : public OutputDispatcherInterface {

public:
    OutputDispatcher(std::string validDataPath = "valid_data.csv", std::string alarmsPath = "alarms.log")
        : m_validDataPath(std::move(validDataPath)),
          m_alarmsPath(std::move(alarmsPath)) {};

    // first idea --> considering the batches are organized in time stamps
    // we can just pass the whole telemetry batch from the RuleEngine here.
    void appendValidData(const MeasDatabaseInterface& db, std::optional<int64_t> timestamp) override;

    // here together with the batch we need to pass the results as well.
    // maybe this is not the most clean and efficient approach but it works fine.
    void appendAlarms(const MeasDatabaseInterface& db, 
                     const std::vector<std::shared_ptr<BaseRule>>& failed_rules,
                     std::optional<int64_t> timestamp) override;

private:
    // helper method which appends the content specified
    void appendLine(const std::string &path, const std::string &data);

    static std::string timeToString(std::optional<int64_t> timestamp);
    static std::string priorityToString(RulePriority p);

    std::string m_validDataPath;
    std::string m_alarmsPath;
    std::mutex  m_mutex;
};

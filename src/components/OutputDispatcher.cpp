#include "OutputDispatcher.h"

// Headers for forward-declared classes
#include "../interfaces/MeasDatabaseInterface.h"
#include "../types/rules/BaseRule.h"
#include "../interfaces/OutputDispatcherInterface.h"

#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

// helper function to convert timestamp into a string
static std::string timeToString(std::optional<int64_t> timestamp) {
    std::string timestamp_str;

    // asked Gemini about timestamp convertion and this blob code came out take it as it is.
    if (timestamp.has_value()) {
        const std::time_t ts = static_cast<std::time_t>(*timestamp);
        std::tm utc_tm{};
        if (gmtime_r(&ts, &utc_tm) != nullptr) {
            std::ostringstream oss;
            oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
            timestamp_str = oss.str();
        } else {
            timestamp_str = std::to_string(*timestamp);
        }
    }

    return timestamp_str;
}

// helper function to map priorities to strings
static std::string priorityToString(RulePriority p) {
    switch (p) {
        case RulePriority::LOW: return "LOW";
        case RulePriority::MEDIUM: return "MEDIUM";
        case RulePriority::HIGH: return "HIGH";
        default: return "UNKNOWN";
    }
}

void OutputDispatcher::appendValidData(const MeasDatabaseInterface& db, std::optional<int64_t> timestamp) {
    std::string data_line;
    // Format: TIMESTAMP;NOMINAL;[SENSOR_1]:[VALUE_1]|[SENSOR_2]:[VALUE_2]|...
    // TODO: we are missing the timestamp in the DB, which has to be changed 
    // as we discussed. I am now printing just the placeholder

    // no need to have a timestamp in the db, we can just pass it as an argument to the function
    // the timestamp is already implicitly present in the db since all the measurements are "emplaced_back"
    // in order.

    std::string timestamp_str = timeToString(timestamp);
    
    // save the converted timestamp
    data_line += timestamp_str + ";NOMINAL;";
    const auto& meas_history = db.getMeasHistory();
    for (const auto& kv : meas_history) {
        const std::string& sensor_id = kv.first;
        const std::vector<double>& values = kv.second;
        for (const double& value : values) {
            data_line += sensor_id + ":" + std::to_string(value) + "|"; 
        }
    }
    appendLine(m_validDataPath, data_line);
}

void OutputDispatcher::appendAlarms(const MeasDatabaseInterface& db, 
                     const std::vector<std::shared_ptr<BaseRule>>& failed_rules,
                     std::optional<int64_t> timestamp) {
    std::string alarm_line; // defined a new variable just for clearence
    // asked Gemini about timestamp convertion and this blob code came out take it as it is.

    std::string timestamp_str = timeToString(timestamp);

    // Format: TIMESTAMP;RULE_ID;PRIORITY;VIOLATED_SENSOR(S);CURRENT_VALUE(S)
    // (Note: For correlation rules, list all sensors and values involved in the parent
    // rules).
    const auto& meas_history = db.getMeasHistory();

    alarm_line += timestamp_str; // Placeholder for timestamp
    for (const auto& rule : failed_rules) {
        alarm_line += rule->getRuleId() + ";" + priorityToString(rule->getPriority()) + ";";
        std::string violated_sensor;
        std::string current_values;
        // I have to reason about how to get the violated sensors and their current values
        // because it is consistent for three rules out of the four, 
        // but the correlation rule differes. How to handle this without an horrible 
        // if-else structure? 
        // Moreover, does it make any sense to have the OutputDispatcher 
        // know about the structure of the rules?

        // TODO: finish the implementation!

    }
}

/** @brief method that appends to the given path the content of the
 * data string, which was previously composed in the methods:
 *  - appendValidData()
 *  - appendAlarms()
 * 
 */
void OutputDispatcher::appendLine(const std::string &path, const std::string &data) {

    std::lock_guard<std::mutex> lock(m_mutex);
    std::ofstream out(path, std::ios::app); // OPEN THE STREAM IN APPEND MODE
    if (!out.is_open()) {
        return;
    }

    out << data; // redirect the string to the output stream
    
}

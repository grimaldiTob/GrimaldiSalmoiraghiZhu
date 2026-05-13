#include "OutputDispatcher.h"

#include <fstream>

void OutputDispatcher::appendValidData(const TelemetryBatch& batch) {
    // logic to implement yet
}

void OutputDispatcher::appendAlarms(const TelemetryBatch& batch, 
                     const std::unordered_map<std::string, std::optional<bool>>& results) {
    // logic to implement yet
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

#include "MeasDatabase.h"

MeasDatabase::MeasDatabase() = default;

/** @brief Searches for sensor id in the measurement database and
 * stores the corresponding value
 */
void MeasDatabase::storeResult(const std::string &sensor_id, double value) {
    // Store value for existing sensor or create a new entry.
    if (m_measurementsHistory[sensor_id].size() >= MAXIMUM_SIZE) {
        clearMeasurements(sensor_id, MAXIMUM_SIZE / 2);
    }
    m_measurementsHistory[sensor_id].emplace_back(value);
}

/** @brief clears the n measurements that arrived first in the
 * database.
 */
void MeasDatabase::clearMeasurements(const std::string &sensor_id, int n) {
    auto it = m_measurementsHistory.find(sensor_id);
    if (it == m_measurementsHistory.end()) {
        return;
    }

    std::vector<double> &vec = it->second;
    if (vec.empty()) {
        return;
    }

    if (static_cast<size_t>(n) >= vec.size()) {
        vec.clear();
        return;
    }
    vec.erase(vec.begin(), vec.begin() + n);
}

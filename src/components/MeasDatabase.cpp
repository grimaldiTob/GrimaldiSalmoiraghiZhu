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
 *
 * IDEA: erase n values for all sensors??? Or just erase n values for a given
 * sensor_id??? I suppose we will get more measurements from one sensor becuase
 * of packets filtering. What if some sensors has undreds of measurements and
 * some have just a few?
 * Luca: Makes sense, should we decide to remove a fixed treshold of
 * measurements from the database,starting from the sensors which have more
 * measurements?
 */
void MeasDatabase::clearMeasurements(const std::string &sensor_id, int n) {
    /*
        ok actually I tought about using the deque structure. But this is not
        ideal since we usually will access more than 1 element to check a
       statefull rule. A deque allows to retrieve/erase first and last element
       in a vector. But clearly this is not our use case + we expect to call
       this method much less than the evaluation of the single rule.

        OUTCOME: we stick with the vector.
    */
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

    /*
    for (auto &kv : m_measurementsHistory) {
        std::vector<double> &vec = kv.second; // need a reference to the object
        vec.erase(vec.begin(), vec.begin() + n); // erase the first n elements
    }
    */
}

#include "../interfaces/MeasDatabaseInterface.h"

// introduce interface
class MeasDatabase : public MeasDatabaseInterface {
  public:
    MeasDatabase();

    const std::unordered_map<std::string, std::vector<double>> &
    getMeasHistory() const override {
        return m_measurementsHistory;
    }

    // access the unordered map at the "sensor_id" key and save the value
    void storeResult(const std::string &sensor_id, double value) override;

    // at some point we need to clean the unordered map
    void clearMeasurements(const std::string &sensor_id, int n = 32) override;

  private:
    const int MAXIMUM_SIZE = 64;
    // ok the idea here --> are we storing all the results or just the results
    // associated to stateful rules? Luca: there is no need to fill the RAM with
    // useless data, so we can indeed store only the results associated to
    // stateful rules.
    std::unordered_map<std::string, std::vector<double>>
        m_measurementsHistory; // our database of results
};

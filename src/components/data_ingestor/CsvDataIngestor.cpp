#include "CsvDataIngestor.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::vector<std::string>
CsvDataIngestor::splitCSVRow(const std::string &line) const {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        // Strip trailing \r to handle Windows (CRLF) line endings
        if (!field.empty() && field.back() == '\r')
            field.pop_back();
        fields.push_back(field);
    }
    return fields;
}

// == Core parsing
// ====================================================================

void CsvDataIngestor::parseTelemetry(const std::string &filename) {
    m_validBatch.clear();

    std::ifstream infile(filename);
    if (!infile.is_open())
        throw std::runtime_error("DataIngestor error: cannot open file: " +
                                 filename);

    // Peek immediately to catch a completely empty file
    if (infile.peek() == std::ifstream::traits_type::eof())
        throw std::runtime_error("DataIngestor error: file is empty: " +
                                 filename);

    int valid_pkg = 0;
    int invalid_pkg = 0;
    std::string line;

    // Skip the header row (timestamp,sensor_id,value,priority)
    std::getline(infile, line);

    // Column indices — must match the header
    constexpr int COL_TIMESTAMP = 0;
    constexpr int COL_SENSOR_ID = 1;
    constexpr int COL_VALUE = 2;
    constexpr int COL_PRIORITY = 3;
    constexpr int EXPECTED_COLS = 4;

    while (std::getline(infile, line)) {
        if (line.empty() || line == "\r")
            continue;

        const auto fields = splitCSVRow(line);

        // ── Validate field count ──────────────────────────────────────────
        if (static_cast<int>(fields.size()) < EXPECTED_COLS - 1) {
            // Need at least timestamp, sensor_id, value (priority is optional)
            std::cerr << "DataIngestor: skipping row with too few fields: \""
                      << line << "\"\n";
            invalid_pkg++;
            continue;
        }

        // ── Validate mandatory fields are not empty ───────────────────────
        const std::string &timestamp_str = fields[COL_TIMESTAMP];
        const std::string &sensor_id = fields[COL_SENSOR_ID];
        const std::string &value_str = fields[COL_VALUE];

        if (timestamp_str.empty() || sensor_id.empty() || value_str.empty()) {
            std::cerr << "DataIngestor: skipping row with empty mandatory "
                         "field: \""
                      << line << "\"\n";
            invalid_pkg++;
            continue;
        }

        // ── Parse numeric value ───────────────────────────────────────────
        double value = 0.0;
        try {
            std::size_t chars_read = 0;
            value = std::stod(value_str, &chars_read);
            if (chars_read != value_str.size()) // trailing garbage
                throw std::invalid_argument("trailing characters");
        } catch (const std::exception &) {
            std::cerr << "DataIngestor: skipping row with invalid value \""
                      << value_str << "\": \"" << line << "\"\n";
            invalid_pkg++;
            continue;
        }

        // ── Parse timestamp ───────────────────────────────────────────────
        const int64_t timestamp_epoch = parseISO8601(timestamp_str);
        if (timestamp_epoch == 0) {
            std::cerr << "DataIngestor: skipping row with invalid timestamp \""
                      << timestamp_str << "\": \"" << line << "\"\n";
            invalid_pkg++;
            continue;
        }

        // ── Parse optional priority field (defaults to LOW = 0) ───────────
        int priority = 0;
        if (static_cast<int>(fields.size()) > COL_PRIORITY &&
            !fields[COL_PRIORITY].empty()) {
            priority = parsePriority(fields[COL_PRIORITY]);
            if (priority == -1) {
                std::cerr << "DataIngestor: skipping row with unrecognised "
                             "priority \""
                          << fields[COL_PRIORITY] << "\": \"" << line << "\"\n";
                invalid_pkg++;
                continue;
            }
        }

        m_validBatch.emplaceBack(sensor_id, timestamp_epoch, value, priority);
        valid_pkg++;
    }

    std::cout << "DataIngestor: valid=" << valid_pkg
              << "  invalid=" << invalid_pkg << "\n";
    printTelemetry(m_validBatch);

    sendValidBatchToAccumulator();
}

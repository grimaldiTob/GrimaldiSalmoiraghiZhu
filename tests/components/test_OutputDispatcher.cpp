#include <catch2/catch_test_macros.hpp>

#include "../../src/components/OutputDispatcher.h"
#include "../../src/interfaces/MeasDatabaseInterface.h"
#include "../../src/types/rules/BaseRule.h"

#include <fstream>
#include <filesystem>
#include <sstream>

// Minimal stub for MeasDatabaseInterface used by OutputDispatcher tests
class MockMeasDatabase : public MeasDatabaseInterface {
public:
    MockMeasDatabase(std::unordered_map<std::string, std::vector<double>> data)
        : data_(std::move(data)) {}

    const std::unordered_map<std::string, std::vector<double>>& getMeasHistory() const override {
        return data_;
    }

    void storeResult(std::string, double) override {}
    void clearMeasurements(int) override {}

private:
    std::unordered_map<std::string, std::vector<double>> data_;
};

// Minimal stub rule deriving from BaseRule
class StubRule : public BaseRule {
public:
    StubRule(const std::string& id, RulePriority p, std::vector<std::string> sensors)
        : BaseRule(id, RuleType::SIMPLE, p), sensors_(std::move(sensors)) {}

    std::optional<bool> evaluate(const TelemetryBatch&, std::unordered_map<std::string, std::optional<bool>>&) override {
        return std::optional<bool>{false};
    }

    std::vector<std::string> getInvolvedSensors() const override {
        return sensors_;
    }

private:
    std::vector<std::string> sensors_;
};

// Minimal stab rule deriving from LogicalCorrelationRule for testing alarms with multiple sensors
class StubCorrelationRule : public BaseRule {
public:
    StubCorrelationRule(const std::string& id, RulePriority p, std::vector<std::string> sensors)
        : BaseRule(id, RuleType::CORRELATION, p), sensors_(std::move(sensors)) {}
    std::optional<bool> evaluate(const TelemetryBatch&, std::unordered_map<std::string, std::optional<bool>>&) override {
        return std::optional<bool>{false};
    }
    std::vector<std::string> getInvolvedSensors() const override {
        return sensors_;
    }

private:
    std::vector<std::string> sensors_;

};

static std::string readFile(const std::string& path) {
    std::ifstream in(path);
    REQUIRE(in.is_open());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

TEST_CASE("appendValidData writes latest values and newline", "[OutputDispatcher]") {
    const std::string validPath = "valid_data_test.csv";
    const std::string alarmsPath = "alarms_test.log";

    // remove old files if present
    std::filesystem::remove(validPath);
    std::filesystem::remove(alarmsPath);

    OutputDispatcher dispatcher(validPath, alarmsPath);

    MockMeasDatabase db({
        {"S1", {1.0, 2.5}},
        {"S2", {3.14}}
    });

    dispatcher.appendValidData(db, std::nullopt);

    std::string content = readFile(validPath);

    // Expect latest values only (S1 -> 2.5, S2 -> 3.14) and trailing newline
    REQUIRE(content.find("S1:2.500000|") != std::string::npos);
    REQUIRE(content.find("S2:3.140000|") != std::string::npos);
    REQUIRE(!content.empty());
    REQUIRE(content.back() == '\n');

    // Remove test files
    std::filesystem::remove(validPath);
    std::filesystem::remove(alarmsPath);
}

TEST_CASE("appendAlarms writes one alarm per rule with separators and sensor values", "[OutputDispatcher]") {
    const std::string validPath = "valid_data_test.csv";
    const std::string alarmsPath = "alarms_test.log";

    // remove old files if present
    std::filesystem::remove(alarmsPath);
    std::filesystem::remove(validPath);

    OutputDispatcher dispatcher(validPath, alarmsPath);

    MockMeasDatabase db({
        {"S1", {1.0, 2.5}},
        {"S2", {3.14}},
        {"S3", {}}
    });

    std::vector<std::shared_ptr<BaseRule>> failed_rules;
    failed_rules.push_back(std::make_shared<StubRule>("R1", RulePriority::HIGH, std::vector<std::string>{"S1"}));
    failed_rules.push_back(std::make_shared<StubRule>("R2", RulePriority::LOW, std::vector<std::string>{"S2", "S3"}));

    dispatcher.appendAlarms(db, failed_rules, std::nullopt);

    std::string content = readFile(alarmsPath);
    REQUIRE(!content.empty());

    // Split into lines
    std::vector<std::string> lines;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) lines.push_back(line);
    }

    // Expect two alarm lines (one per failed rule)
    REQUIRE(lines.size() == 2);

    // Check first alarm contains R1 and HIGH and S1 value
    REQUIRE(lines[0].find(";R1;HIGH;") != std::string::npos);
    REQUIRE(lines[0].find("S1:2.500000|") != std::string::npos);

    // Check second alarm contains R2 and LOW and S2 value and S3 N/A
    REQUIRE(lines[1].find(";R2;LOW;") != std::string::npos);
    REQUIRE(lines[1].find("S2:3.140000|") != std::string::npos);
    REQUIRE(lines[1].find("S3:N/A|") != std::string::npos);

    // Remove test files
    std::filesystem::remove(alarmsPath);
    std::filesystem::remove(validPath);
}

TEST_CASE("appendAlarms with correlation rule includes all involved sensors", "[OutputDispatcher]") {
    const std::string validPath = "valid_data_test.csv";
    const std::string alarmsPath = "alarms_test.log";

    // remove old files if present
    std::filesystem::remove(alarmsPath);
    std::filesystem::remove(validPath);

    OutputDispatcher dispatcher(validPath, alarmsPath);

    MockMeasDatabase db({
        {"S1", {1.0}},
        {"S2", {3.14}},
        {"S3", {2.71}}
    });

    std::vector<std::shared_ptr<BaseRule>> failed_rules;
    failed_rules.push_back(std::make_shared<StubCorrelationRule>("R_CORR", RulePriority::MEDIUM, std::vector<std::string>{"S1", "S2", "S3"}));

    dispatcher.appendAlarms(db, failed_rules, std::nullopt);

    std::string content = readFile(alarmsPath);
    REQUIRE(!content.empty());

    // Split into lines
    std::vector<std::string> lines;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) lines.push_back(line);
    }

    // Expect one alarm line for the correlation rule
    REQUIRE(lines.size() == 1);
    REQUIRE(lines[0].find(";R_CORR;MEDIUM;") != std::string::npos);
    REQUIRE(lines[0].find("S1:1.000000|") != std::string::npos);
    REQUIRE(lines[0].find("S2:3.140000|") != std::string::npos);
    REQUIRE(lines[0].find("S3:2.710000|") != std::string::npos);

    // Remove test files
    std::filesystem::remove(alarmsPath);
    std::filesystem::remove(validPath);
}

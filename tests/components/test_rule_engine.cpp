#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <optional>
#include <memory>

#include "../../src/components/RuleEngine.h"
#include "../../src/components/ThreadSafeBuffer.h"
#include "../../src/types/rules/SimpleRule.h"
#include "../../src/types/TelemetryBatch.h"

#include "../../src/interfaces/RuleLoaderInterface.h"
#include "../../src/interfaces/OutputDispatcherInterface.h"
#include "../../src/interfaces/MeasDatabaseInterface.h"

#include "../../external/simdjson.h"

// Mocks for all the interfaces needed bu the RuleEngine
// I still have a question: do we need to mock even the rules?
// For now I have been using the the real Rule (which we have tested and know that are fine), 
// but does this make sense? Or is this test already considered an integration? 
// The point is though that rules are somehow the "core" of the application 
// as much as the rule engine, so I expect them to either work together or not work at all...
class FakeRuleLoader : public RuleLoaderInterface {
public:
    void loadRules(simdjson::ondemand::parser& /*parser*/, 
        const std::string& /*filename*/, std::vector<std::shared_ptr<BaseRule>>& rules_list) override {
        rules_list.emplace_back(std::make_shared<SimpleRule>("r1", RulePriority::HIGH, "s1", ">", 10.0)); 
        // Already adding a simple rule to the list.
        // We surely have to add more to test more complex scenarios. 
    }
};

class FakeMeasDB : public MeasDatabaseInterface {
public:
    const std::unordered_map<std::string, std::vector<double>>& getMeasHistory() const override { return history; }
    void storeResult(std::string sensor, double value) override { history[sensor].push_back(value); }
    void clearMeasurements(int /*n*/ = 32) override { history.clear(); }

private:
    std::unordered_map<std::string, std::vector<double>> history;
};

class FakeOutputDispatcher : public OutputDispatcherInterface {
public:
    void appendValidData(const MeasDatabaseInterface& /*db*/) override { validCalls++; }
    void appendAlarms(const MeasDatabaseInterface& /*db*/, const std::vector<std::shared_ptr<BaseRule>>& /*failed*/) override { alarmCalls++; }

    int validCalls{0};
    int alarmCalls{0};
};

TEST_CASE("RuleEngine processes batches correctly", "[RuleEngine]") {
    ThreadSafeBuffer<TelemetryBatch> buffer(4);
    FakeMeasDB db;
    FakeOutputDispatcher dispatcher;

    // Initialize engine with an initial timestamp of 1 so first collected
    // measurements are evaluated when timestamp changes.
    RuleEngine engine(buffer, db, std::optional<int64_t>(1));
    engine.setOutputDispatcher(dispatcher);

    FakeRuleLoader loader;
    simdjson::ondemand::parser parser;
    engine.setRulesList(loader, parser);

    // Build a batch containing two measurements with different timestamps.
    TelemetryBatch bat(2);
    bat.emplaceBack("s1", 1, 20.0, 1); // should evaluate true against >10
    bat.emplaceBack("s1", 2, 5.0, 1);  // should evaluate false against >10

    buffer.push(bat);
    buffer.finish_production();

    engine.run();

    // One successful evaluation should have triggered appendValidData.
    REQUIRE(dispatcher.validCalls == 1);
    REQUIRE(dispatcher.alarmCalls == 0);

    // Both measurements are stored in the fake DB.
    const auto& hist = db.getMeasHistory();
    REQUIRE(hist.count("s1") == 1);
    REQUIRE(hist.at("s1").size() == 2);
    REQUIRE(hist.at("s1")[0] == Catch::Approx(20.0));
    REQUIRE(hist.at("s1")[1] == Catch::Approx(5.0));
}

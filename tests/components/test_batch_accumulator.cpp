#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include "../../src/components/BatchAccumulator.h"
#include "../../src/interfaces/ProducerBuffer.h"
#include "../../src/types/TelemetryBatch.h"


// Mock the queue behavior
class MockBroker : public ProducerBuffer<TelemetryBatch> {
public:
    std::vector<TelemetryBatch> queue;
    bool finishCalled = false;

    void push(TelemetryBatch item) override {
        queue.push_back(std::move(item));
    }

    void finish_production() override {
        finishCalled = true;
    }

    size_t pushCount() const { return queue.size(); }
};

/** Build a TelemetryBatch with `count` measurements.
 *  sensor name  = "s<i>", timestamp = i, value = i * 1.0, priority = 1
 */
static TelemetryBatch makeBatch(size_t count, int64_t baseTimestamp = 0, int priority = 1) {
    TelemetryBatch b;
    for (size_t i = 0; i < count; ++i) {
        b.emplaceBack("s" + std::to_string(i),
                      baseTimestamp + static_cast<int64_t>(i),
                      static_cast<double>(i),
                      priority);
    }
    return b;
}

/** Verify that the n-th pushed batch contains the expected sensor names in order. */
static void requireSensorNames(const MockBroker& broker, size_t batchIndex, const std::vector<std::string>& expected) {

    // Since our goal is to test if the accumulator correctly feed the queue, 
    // we ASSUME that we have just ONE PRODUCER (accumulator) and 
    // the queue can be ONLY populated (no leaving packets) 
    
    // If the broker has more items than the accumulator has send it, the test fail
    REQUIRE(broker.pushCount() > batchIndex); 
    
    // Compare the ids of a specific batch in the queue
    const auto& actual = broker.queue[batchIndex].sensors_name;
    REQUIRE(actual == expected);
}

/*================================== TEST CASES ====================================*/

TEST_CASE("BatchAccumulator construction", "[BatchAccumulator]") {

    SECTION("Default batch size is 100") {
        MockBroker broker;
        BatchAccumulator acc(broker);
        REQUIRE(acc.getBatchSize() == 100);
    }

    SECTION("Custom batch size is stored correctly") {
        MockBroker broker;
        BatchAccumulator acc(broker, 42);
        REQUIRE(acc.getBatchSize() == 42);
    }

    SECTION("Internal batch file starts empty") {
        MockBroker broker;
        BatchAccumulator acc(broker, 10);
        REQUIRE(acc.getBatchFile().getSize() == 0);
    }
}


TEST_CASE("storeValidData — accumulation below threshold", "[BatchAccumulator]") {

    SECTION("No push when validBatch is smaller than batchSize") {
        MockBroker broker;
        BatchAccumulator acc(broker, 10);

        auto incomingBatch = makeBatch(5);
        acc.storeValidData(incomingBatch);

        // No batch is sent to the accumulator yet
        REQUIRE(broker.pushCount() == 0);
        REQUIRE(acc.getBatchFile().getSize() == 5);
    }

    SECTION("Repeated calls below threshold accumulate correctly") {
        MockBroker broker;
        BatchAccumulator acc(broker, 10);

        TelemetryBatch batch = makeBatch(3);
        // Accumulate three times the same batch
        acc.storeValidData(batch);
        acc.storeValidData(batch);
        acc.storeValidData(batch);

        REQUIRE(broker.pushCount() == 0);       // Not send it yet
        REQUIRE(acc.getBatchFile().getSize() == 9);
    }
}

TEST_CASE("storeValidData — exact threshold triggers a single flush", "[BatchAccumulator]") {

    MockBroker broker;
    constexpr size_t BATCH_SIZE = 5;
    BatchAccumulator acc(broker, BATCH_SIZE);

    auto incomingBatch = makeBatch(BATCH_SIZE);
    acc.storeValidData(incomingBatch);

    REQUIRE(broker.pushCount() == 1);

    SECTION("Internal batch is cleared after flush") {
        REQUIRE(acc.getBatchFile().getSize() == 0);
    }

    SECTION("Pushed batch has exactly batchSize elements") {
        REQUIRE(broker.queue[0].getSize() == BATCH_SIZE);
    }

    SECTION("Pushed batch preserves measurement data") {
        // The flush snapshot must carry the correct sensor names
        std::vector<std::string> expected;
        for (size_t i = 0; i < BATCH_SIZE; ++i)
            expected.push_back("s" + std::to_string(i));

        requireSensorNames(broker, 0, expected);
    }
}


TEST_CASE("storeValidData — overflow triggers multiple flushes in one call", "[BatchAccumulator]") {

    MockBroker broker;
    constexpr size_t BATCH_SIZE = 3;
    BatchAccumulator acc(broker, BATCH_SIZE);

    // 7 measurements → 2 full flushes (at index 2 and 5), 1 leftover
    auto incomingBatch = makeBatch(7);
    acc.storeValidData(incomingBatch);

    REQUIRE(broker.pushCount() == 2);
    REQUIRE(acc.getBatchFile().getSize() == 1);

    SECTION("Each flushed batch has exactly batchSize elements") {
        REQUIRE(broker.queue[0].getSize() == BATCH_SIZE);
        REQUIRE(broker.queue[1].getSize() == BATCH_SIZE);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("storeValidData — carry-over semantics across multiple calls", "[BatchAccumulator]") {

    MockBroker broker;
    constexpr size_t BATCH_SIZE = 4;
    BatchAccumulator acc(broker, BATCH_SIZE);
    TelemetryBatch batchWith3 = makeBatch(3);
    TelemetryBatch batchWith2 = makeBatch(2);

    // First call: 3 elements — not enough to flush
    acc.storeValidData(batchWith3);
    REQUIRE(broker.pushCount() == 0);
    REQUIRE(acc.getBatchFile().getSize() == 3);

    // Second call: 2 elements — total becomes 5, one flush at element 4, one leftover
    acc.storeValidData(batchWith2);
    REQUIRE(broker.pushCount() == 1);
    REQUIRE(acc.getBatchFile().getSize() == 1);

    // Third call: 3 more elements — total becomes 4, triggers another flush
    acc.storeValidData(batchWith3);
    REQUIRE(broker.pushCount() == 2);
    REQUIRE(acc.getBatchFile().getSize() == 0);
}

// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("storeValidData — measurement fields are forwarded correctly", "[BatchAccumulator]") {

    MockBroker broker;
    BatchAccumulator acc(broker, 2);

    TelemetryBatch incomingBatch;
    incomingBatch.emplaceBack("temperature", 1000, 36.6, 2);
    incomingBatch.emplaceBack("pressure",    1001, 1013.25, 1);
    acc.storeValidData(incomingBatch);

    REQUIRE(broker.pushCount() == 1);
    const auto& pushed = broker.queue[0];

    SECTION("Sensor names are preserved") {
        REQUIRE(pushed.sensors_name[0] == "temperature");
        REQUIRE(pushed.sensors_name[1] == "pressure");
    }

    SECTION("Timestamps are preserved") {
        REQUIRE(pushed.timestamps[0] == 1000);
        REQUIRE(pushed.timestamps[1] == 1001);
    }

    SECTION("Values are preserved") {
        REQUIRE(pushed.values[0] == Catch::Approx(36.6));
        REQUIRE(pushed.values[1] == Catch::Approx(1013.25));
    }

    SECTION("Priorities are preserved") {
        REQUIRE(pushed.priorities[0] == 2);
        REQUIRE(pushed.priorities[1] == 1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("storeValidData — flush order is FIFO", "[BatchAccumulator]") {

    MockBroker broker;
    BatchAccumulator acc(broker, 3);

    // Push two separate waves; the first three sensors must appear in the first flush.
    TelemetryBatch wave1;
    wave1.emplaceBack("alpha",   0, 1.0, 1);
    wave1.emplaceBack("beta",    1, 2.0, 1);
    wave1.emplaceBack("gamma",   2, 3.0, 1);  // triggers flush
    wave1.emplaceBack("delta",   3, 4.0, 1);  // stays in buffer
    acc.storeValidData(wave1);

    REQUIRE(broker.pushCount() == 1);
    REQUIRE(broker.queue[0].sensors_name[0] == "alpha");
    REQUIRE(broker.queue[0].sensors_name[1] == "beta");
    REQUIRE(broker.queue[0].sensors_name[2] == "gamma");

    // delta + two more should form the second flush
    TelemetryBatch wave2;
    wave2.emplaceBack("epsilon", 4, 5.0, 1);
    wave2.emplaceBack("zeta",    5, 6.0, 1);
    acc.storeValidData(wave2);

    REQUIRE(broker.pushCount() == 2);
    REQUIRE(broker.queue[1].sensors_name[0] == "delta");
    REQUIRE(broker.queue[1].sensors_name[1] == "epsilon");
    REQUIRE(broker.queue[1].sensors_name[2] == "zeta");
}

// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("storeValidData — batch size of 1 flushes on every element", "[BatchAccumulator]") {

    MockBroker broker;
    BatchAccumulator acc(broker, 1);

    auto incomingBatch = makeBatch(5);
    acc.storeValidData(incomingBatch);

    REQUIRE(broker.pushCount() == 5);
    REQUIRE(acc.getBatchFile().getSize() == 0);

    // Each flushed batch must carry exactly one measurement
    for (size_t i = 0; i < 5; ++i) {
        REQUIRE(broker.queue[i].getSize() == 1);
        REQUIRE(broker.queue[i].sensors_name[0] == "s" + std::to_string(i));
    }
}

// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("storeValidData — broker is NOT called when batch never fills", "[BatchAccumulator]") {

    MockBroker broker;
    BatchAccumulator acc(broker, 1000);

    for (int call = 0; call < 10; ++call) {
        TelemetryBatch b = makeBatch(99);
        acc.storeValidData(b);
    }

    // 10 × 99 = 990 < 1000: no flush ever
    REQUIRE(broker.pushCount() == 0);
    REQUIRE(acc.getBatchFile().getSize() == 990);
    REQUIRE(broker.finishCalled == false);
}
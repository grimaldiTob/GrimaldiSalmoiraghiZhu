#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include "../../src/types/TelemetryBatch.h"
#include "../../src/components/ThreadSafeBuffer.h"

// Build a simple batch
static TelemetryBatch makeBatch(const std::string& sensorId,
                                 int64_t ts   = 1000,
                                 double  val  = 42.0,
                                 int     prio = 1) {
    TelemetryBatch batch(1);
    batch.emplaceBack(sensorId, ts, val, prio);
    return batch;
}

// millisecond duration type. 
using ms = std::chrono::milliseconds;

TEST_CASE("ThreadSafeBuffer construction", "[tsb][construction]")
{
    SECTION("buffer with capacity 5")
    {
        ThreadSafeBuffer<TelemetryBatch> buffer(5);
        // Should not throw or block; we can push one item right away.
        REQUIRE_NOTHROW(buffer.push(makeBatch("s1")));
    }
}

TEST_CASE("ThreadSafeBuffer basic push and pop", "[tsb][basic]")
{
    // Create a buffer of capacity 8
    ThreadSafeBuffer<TelemetryBatch> buffer(8);

    SECTION("single push then pop retrieves the same batch")
    {
        TelemetryBatch src = makeBatch("sensor1", 9999, 36.6, 2);
        buffer.push(src);

        // stop the producing process, the batch is still in the queue ready to be processed
        buffer.finish_production(); 

        TelemetryBatch currentBatch;
        bool isFinished = buffer.pop(currentBatch);

        REQUIRE(isFinished);
        REQUIRE(currentBatch.getSize() == 1);
        REQUIRE(currentBatch.sensors_name[0] == "sensor1");
        REQUIRE(currentBatch.timestamps[0] == 9999);
        REQUIRE(currentBatch.values[0] == Catch::Approx(36.6));
        REQUIRE(currentBatch.priorities[0] == 2);
    }

    SECTION("FIFO ordering is preserved across multiple pushes")
    {
        constexpr int N = 5;
        for (int i = 0; i < N; ++i)
            buffer.push(makeBatch("s" + std::to_string(i), i));

        buffer.finish_production();

        for (int i = 0; i < N; ++i) {
            TelemetryBatch batch;
            REQUIRE(buffer.pop(batch));
            REQUIRE(batch.sensors_name[0] == "s" + std::to_string(i));
            REQUIRE(batch.timestamps[0] == i);
        }
    }

    SECTION("pop returns false when buffer is empty after finish_production")
    {
        buffer.finish_production();
        TelemetryBatch b;
        REQUIRE_FALSE(buffer.pop(b));
    }

    SECTION("pop returns false after all items have been drained")
    {
        buffer.push(makeBatch("only"));
        buffer.finish_production();

        TelemetryBatch b;
        REQUIRE(buffer.pop(b));    // consumes the one item
        REQUIRE_FALSE(buffer.pop(b)); // now empty + finished
    }
}

TEST_CASE("ThreadSafeBuffer - move semantics", "[tsb][move]")
{
    ThreadSafeBuffer<TelemetryBatch> buf(4);

    SECTION("batch is moved into and out of the buffer without copying data")
    {
        TelemetryBatch src;
        for (int i = 0; i < 100; ++i)
            src.emplaceBack("sensor_" + std::to_string(i), i, i * 0.5, i % 3);

        const size_t expectedSize = src.getSize();

        buf.push(std::move(src));        // src should be in a valid-but-unspecified state
        buf.finish_production();

        TelemetryBatch dst;
        REQUIRE(buf.pop(dst));
        REQUIRE(dst.getSize() == expectedSize);
        REQUIRE(dst.sensors_name[50] == "sensor_50");
        REQUIRE(dst.values[50]       == Catch::Approx(25.0));
    }
}

TEST_CASE("ThreadSafeBuffer – capacity back-pressure", "[tsb][capacity]")
{
    SECTION("producer blocks when buffer is full and unblocks after a pop")
    {
        ThreadSafeBuffer<TelemetryBatch> buf(2);

        // Fill to capacity synchronously.
        buf.push(makeBatch("a"));
        buf.push(makeBatch("b"));

        std::atomic<bool> pushedThird{false};

        // Third push must block because capacity == 2.
        std::thread producer([&]() {
            buf.push(makeBatch("c"));
            pushedThird.store(true, std::memory_order_release);
        });

        // Give the producer time to block.
        std::this_thread::sleep_for(ms(50));
        REQUIRE_FALSE(pushedThird.load());

        // Free one slot – producer should now unblock.
        TelemetryBatch dummy;
        buf.pop(dummy);

        producer.join();
        REQUIRE(pushedThird.load());

        // Clean up.
        buf.finish_production();
        buf.pop(dummy);
        buf.pop(dummy);
    }
}

TEST_CASE("ThreadSafeBuffer – finish_production unblocks all consumers",
          "[tsb][finish]")
{
    SECTION("two consumers both exit cleanly when production finishes on empty buffer")
    {
        ThreadSafeBuffer<TelemetryBatch> buf(4);

        std::atomic<int> exitCount{0};

        auto consumerFn = [&]() {
            TelemetryBatch b;
            while (buf.pop(b)) { /* drain */ }
            exitCount.fetch_add(1, std::memory_order_release);
        };

        std::thread c1(consumerFn);
        std::thread c2(consumerFn);

        std::this_thread::sleep_for(ms(30)); // let consumers block on pop
        buf.finish_production();

        c1.join();
        c2.join();

        REQUIRE(exitCount.load() == 2);
    }

    SECTION("consumers drain all items before exiting")
    {
        constexpr int N = 10;
        ThreadSafeBuffer<TelemetryBatch> buf(N);

        std::atomic<int> consumed{0};

        std::thread consumer([&]() {
            TelemetryBatch b;
            while (buf.pop(b))
                consumed.fetch_add(static_cast<int>(b.getSize()),
                                   std::memory_order_relaxed);
        });

        for (int i = 0; i < N; ++i)
            buf.push(makeBatch("s" + std::to_string(i)));

        buf.finish_production();
        consumer.join();

        REQUIRE(consumed.load() == N); // each batch had exactly one entry
    }
}

TEST_CASE("ThreadSafeBuffer - SPSC stress", "[tsb][stress][spsc]")
{
    constexpr size_t TOTAL   = 500;
    constexpr size_t CAP     = 16;

    ThreadSafeBuffer<TelemetryBatch> buf(CAP);
    std::vector<std::string> received;
    received.reserve(TOTAL);

    std::thread producer([&]() {
        for (size_t i = 0; i < TOTAL; ++i)
            buf.push(makeBatch("sensor_" + std::to_string(i),
                               static_cast<int64_t>(i)));
        buf.finish_production();
    });

    std::thread consumer([&]() {
        TelemetryBatch b;
        while (buf.pop(b))
            received.push_back(b.sensors_name[0]);
    });

    producer.join();
    consumer.join();

    REQUIRE(received.size() == TOTAL);
    for (size_t i = 0; i < TOTAL; ++i)
        REQUIRE(received[i] == "sensor_" + std::to_string(i));
}

TEST_CASE("ThreadSafeBuffer - MPSC stress", "[tsb][stress][mpsc]")
{
    constexpr size_t PRODUCERS   = 4;
    constexpr size_t PER_PRODUCER = 100;
    constexpr size_t TOTAL        = PRODUCERS * PER_PRODUCER;
    constexpr size_t CAP          = 8;

    ThreadSafeBuffer<TelemetryBatch> buf(CAP);
    std::atomic<int> producersDone{0};
    std::atomic<size_t> totalConsumed{0};

    // Producers
    std::vector<std::thread> producers;
    for (size_t p = 0; p < PRODUCERS; ++p) {
        producers.emplace_back([&, p]() {
            for (size_t i = 0; i < PER_PRODUCER; ++i) {
                auto id = "p" + std::to_string(p) + "_i" + std::to_string(i);
                buf.push(makeBatch(id, static_cast<int64_t>(i),
                                   static_cast<double>(p), static_cast<int>(p)));
            }
            if (producersDone.fetch_add(1) + 1 == static_cast<int>(PRODUCERS))
                buf.finish_production();
        });
    }

    // Single consumer
    std::thread consumer([&]() {
        TelemetryBatch b;
        while (buf.pop(b))
            totalConsumed.fetch_add(b.getSize(), std::memory_order_relaxed);
    });

    for (auto& t : producers) t.join();
    consumer.join();

    REQUIRE(totalConsumed.load() == TOTAL);
}

TEST_CASE("ThreadSafeBuffer - SPMC stress", "[tsb][stress][spmc]")
{
    constexpr size_t CONSUMERS = 4;
    constexpr size_t TOTAL     = 200;
    constexpr size_t CAP       = 16;

    ThreadSafeBuffer<TelemetryBatch> buf(CAP);
    std::atomic<size_t> totalConsumed{0};

    std::vector<std::thread> consumers;
    for (size_t c = 0; c < CONSUMERS; ++c) {
        consumers.emplace_back([&]() {
            TelemetryBatch b;
            while (buf.pop(b))
                totalConsumed.fetch_add(b.getSize(), std::memory_order_relaxed);
        });
    }

    std::thread producer([&]() {
        for (size_t i = 0; i < TOTAL; ++i)
            buf.push(makeBatch("batch_" + std::to_string(i)));
        buf.finish_production();
    });

    producer.join();
    for (auto& t : consumers) t.join();

    REQUIRE(totalConsumed.load() == TOTAL);
}

TEST_CASE("ThreadSafeBuffer TelemetryBatch data integrity round-trip",
          "[tsb][integrity]")
{
    ThreadSafeBuffer<TelemetryBatch> buf(4);

    SECTION("multi-entry batch survives the round-trip intact")
    {
        TelemetryBatch src(3);
        src.emplaceBack("voltage",     100, 230.1, 1);
        src.emplaceBack("current",     200,   9.8, 2);
        src.emplaceBack("temperature", 300,  72.5, 3);

        buf.push(src);
        buf.finish_production();

        TelemetryBatch dst;
        REQUIRE(buf.pop(dst));

        REQUIRE(dst.getSize() == 3);

        REQUIRE(dst.sensors_name[0] == "voltage");
        REQUIRE(dst.timestamps[0]   == 100);
        REQUIRE(dst.values[0]       == Catch::Approx(230.1));
        REQUIRE(dst.priorities[0]   == 1);

        REQUIRE(dst.sensors_name[1] == "current");
        REQUIRE(dst.timestamps[1]   == 200);
        REQUIRE(dst.values[1]       == Catch::Approx(9.8));
        REQUIRE(dst.priorities[1]   == 2);

        REQUIRE(dst.sensors_name[2] == "temperature");
        REQUIRE(dst.timestamps[2]   == 300);
        REQUIRE(dst.values[2]       == Catch::Approx(72.5));
        REQUIRE(dst.priorities[2]   == 3);
    }

    SECTION("cleared batch can be reused and pushed again")
    {
        TelemetryBatch b(1);
        b.emplaceBack("old_sensor", 0, 0.0, 0);
        b.clear();
        REQUIRE(b.getSize() == 0);

        b.emplaceBack("new_sensor", 42, 3.14, 5);
        buf.push(b);
        buf.finish_production();

        TelemetryBatch dst;
        REQUIRE(buf.pop(dst));
        REQUIRE(dst.sensors_name[0] == "new_sensor");
        REQUIRE(dst.timestamps[0]   == 42);
        REQUIRE(dst.values[0]       == Catch::Approx(3.14));
        REQUIRE(dst.priorities[0]   == 5);
    }
}

TEST_CASE("ThreadSafeBuffer  capacity-1 alternating push/pop", "[tsb][edge]")
{
    ThreadSafeBuffer<TelemetryBatch> buf(1);
    constexpr int ROUNDS = 20;
    std::vector<int> order;

    std::thread producer([&]() {
        for (int i = 0; i < ROUNDS; ++i)
            buf.push(makeBatch("e" + std::to_string(i),
                               static_cast<int64_t>(i)));
        buf.finish_production();
    });

    std::thread consumer([&]() {
        TelemetryBatch b;
        while (buf.pop(b))
            order.push_back(static_cast<int>(b.timestamps[0]));
    });

    producer.join();
    consumer.join();

    REQUIRE(order.size() == ROUNDS);
    for (int i = 0; i < ROUNDS; ++i)
        REQUIRE(order[i] == i);
}

#include "../../src/types/TelemetryBatch.h"
#include <catch2/catch_approx.hpp> // Required for floating point comparisons
#include <catch2/catch_test_macros.hpp>

TEST_CASE("TelemetryBatch Initialization", "[TelemetryBatch]") {

    SECTION("Initialize with empty vectors") {
        TelemetryBatch batch;

        REQUIRE(batch.getSize() == 0);
        CHECK(batch.sensors_name.empty());
        CHECK(batch.timestamps.empty());
        CHECK(batch.values.empty());
        CHECK(batch.priorities.empty());
    }

    SECTION("Initialize with reserveSize passed as parameter") {
        const size_t reserveSize = 100;
        TelemetryBatch batch(reserveSize);

        REQUIRE(batch.getSize() == 0);
        CHECK(batch.sensors_name.capacity() >= reserveSize);
        CHECK(batch.timestamps.capacity() >= reserveSize);
        CHECK(batch.values.capacity() >= reserveSize);
        CHECK(batch.priorities.capacity() >= reserveSize);
    }
}

TEST_CASE("TelemetryBatch Data Insertion", "[TelemetryBatch]") {

    TelemetryBatch batch; // shared from all sections

    SECTION("emplaceBack stores a single record") {
        batch.emplaceBack("sensor_1", 1680000000, 42.5, 1);

        REQUIRE(batch.getSize() == 1);
        CHECK(batch.sensors_name[0] == "sensor_1");
        CHECK(batch.timestamps[0] == 1680000000);
        CHECK(batch.values[0] == Catch::Approx(42.5));
        CHECK(batch.priorities[0] == 1);
    }

    SECTION("emplaceBack with multiple records in order") {
        batch.emplaceBack("sensor_1", 1000, 1.1, 1);
        batch.emplaceBack("sensor_2", 2000, 2.2, 2);

        REQUIRE(batch.getSize() == 2);
        CHECK(batch.sensors_name[1] == "sensor_2");
        CHECK(batch.timestamps[1] == 2000);
        CHECK(batch.values[1] == Catch::Approx(2.2));
        CHECK(batch.priorities[1] == 2);
    }
}

TEST_CASE("TelemetryBatch memory cleaning", "[TelemetryBatch]") {
    TelemetryBatch batch;

    SECTION("clear() on an empty batch is safe") {
        REQUIRE_NOTHROW(batch.clear());
        CHECK(batch.getSize() == 0);
    }

    SECTION("clear() resets size and empties all vectors") {
        batch.emplaceBack("sensor_1", 1000, 1.1, 1);
        batch.emplaceBack("sensor_2", 2000, 2.2, 2);
        REQUIRE(batch.getSize() == 2); // safe check

        batch.clear();

        REQUIRE(batch.getSize() == 0);
        CHECK(batch.sensors_name.empty());
        CHECK(batch.timestamps.empty());
        CHECK(batch.values.empty());
        CHECK(batch.priorities.empty());
    }
}

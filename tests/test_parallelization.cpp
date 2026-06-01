#include <mpi.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "../src/components/MpiRuleEngine.h"
#include "../src/components/RuleEngine.h"
#include "../src/components/ThreadSafeBuffer.h"
#include "../src/interfaces/MeasDatabaseInterface.h"
#include "../src/interfaces/OutputDispatcherInterface.h"
#include "../src/interfaces/RuleLoaderInterface.h"
#include "../src/types/rules/SimpleRule.h"

namespace {

struct BenchmarkConfig {
    size_t measurements = 100000;
    size_t batch_size = 1000;
    size_t rules = 1000;
    int iterations = 3;
    std::string engine = "both";
    std::string output_path = "./benchmark_results.txt";
};

struct Stats {
    double avg_ms = 0.0;
    double min_ms = 0.0;
    double max_ms = 0.0;
};

class MockMeasDatabase : public MeasDatabaseInterface {
  public:
    const std::unordered_map<std::string, std::vector<double>> &
    getMeasHistory() const override {
        return history;
    }

    void storeResult(const std::string &, double) override {}

    void clearMeasurements(const std::string &, int) override {}

  private:
    std::unordered_map<std::string, std::vector<double>> history;
};

class MockOutputDispatcher : public OutputDispatcherInterface {
  public:
    void appendValidData(const MeasDatabaseInterface &,
                         std::optional<int64_t>) override {}

    void appendAlarms(const MeasDatabaseInterface &,
                      const std::vector<std::shared_ptr<BaseRule>> &,
                      std::optional<int64_t>) override {}
};

class BenchmarkRuleLoader : public RuleLoaderInterface {
  public:
    BenchmarkRuleLoader(size_t rule_count,
                        const std::vector<std::string> &sensors)
        : rule_count(rule_count), sensors(sensors) {}

    void
    loadRules(const std::string &,
              std::vector<std::shared_ptr<BaseRule>> &rules_list) override {
        rules_list.reserve(rule_count);
        const size_t sensor_count = std::max<size_t>(1, sensors.size());

        for (size_t i = 0; i < rule_count; ++i) {
            RulePriority priority = RulePriority::HIGH;
            if (i % 3 == 1) {
                priority = RulePriority::MEDIUM;
            } else if (i % 3 == 2) {
                priority = RulePriority::LOW;
            }

            const std::string &sensor_id = sensors[i % sensor_count];
            const double threshold = 50.0 + static_cast<double>(i % 5);
            rules_list.emplace_back(std::make_shared<SimpleRule>(
                "r" + std::to_string(i), priority, sensor_id, ">", threshold));
        }
    }

  private:
    size_t rule_count;
    const std::vector<std::string> &sensors;
};

std::vector<std::string> makeSensorIds(size_t count) {
    std::vector<std::string> sensors;
    sensors.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        sensors.emplace_back("s" + std::to_string(i));
    }
    return sensors;
}

std::vector<TelemetryBatch>
makeBatches(size_t measurements, size_t batch_size,
            const std::vector<std::string> &sensors) {
    std::vector<TelemetryBatch> batches;
    if (measurements == 0 || batch_size == 0 || sensors.empty()) {
        return batches;
    }

    const size_t batch_count = (measurements + batch_size - 1) / batch_size;
    batches.reserve(batch_count);

    size_t remaining = measurements;
    size_t idx = 0;
    const int64_t base_ts = 1;

    for (size_t b = 0; b < batch_count; ++b) {
        const size_t current = std::min(batch_size, remaining);
        TelemetryBatch batch(current);
        const int64_t timestamp = base_ts + static_cast<int64_t>(b);

        for (size_t i = 0; i < current; ++i, ++idx) {
            const std::string &sensor = sensors[idx % sensors.size()];
            const double value = static_cast<double>(idx % 100);
            batch.emplaceBack(sensor, timestamp, value, 1);
        }

        batches.emplace_back(std::move(batch));
        remaining -= current;
    }

    return batches;
}

void fillBuffer(ThreadSafeBuffer<TelemetryBatch> &buffer,
                const std::vector<TelemetryBatch> &batches) {
    for (const auto &batch : batches) {
        buffer.push(batch);
    }
    buffer.finish_production();
}

Stats computeStats(const std::vector<double> &times_ms) {
    Stats stats;
    if (times_ms.empty()) {
        return stats;
    }

    double sum = 0.0;
    stats.min_ms = times_ms.front();
    stats.max_ms = times_ms.front();
    for (double t : times_ms) {
        sum += t;
        stats.min_ms = std::min(stats.min_ms, t);
        stats.max_ms = std::max(stats.max_ms, t);
    }
    stats.avg_ms = sum / static_cast<double>(times_ms.size());
    return stats;
}

std::optional<int64_t>
initialTimestamp(const std::vector<TelemetryBatch> &batches) {
    if (batches.empty() || batches.front().timestamps.empty()) {
        return std::nullopt;
    }
    return batches.front().timestamps.front();
}

double runOmpOnce(const BenchmarkConfig &config,
                  const std::vector<TelemetryBatch> &batches,
                  const std::vector<std::string> &sensors) {
    ThreadSafeBuffer<TelemetryBatch> buffer(batches.size() + 1);
    MockMeasDatabase db;
    MockOutputDispatcher dispatcher;
    BenchmarkRuleLoader loader(config.rules, sensors);

    RuleEngine engine(buffer, db, dispatcher, loader,
                      initialTimestamp(batches));
    engine.setRulesList();
    fillBuffer(buffer, batches);

    auto start = std::chrono::steady_clock::now();
    engine.run();
    auto end = std::chrono::steady_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = end - start;
    return elapsed.count();
}

double runMpiOnce(const BenchmarkConfig &config,
                  const std::vector<TelemetryBatch> &batches,
                  const std::vector<std::string> &sensors, MPI_Comm comm) {
    ThreadSafeBuffer<TelemetryBatch> buffer(batches.size() + 1);
    MockMeasDatabase db;
    MockOutputDispatcher dispatcher;
    BenchmarkRuleLoader loader(config.rules, sensors);

    MpiRuleEngine engine(buffer, db, dispatcher, loader,
                         initialTimestamp(batches), comm);
    engine.setRulesList();
    fillBuffer(buffer, batches);

    MPI_Barrier(comm);
    const double start = MPI_Wtime();
    engine.run();
    const double elapsed = MPI_Wtime() - start;

    double max_elapsed = 0.0;
    MPI_Reduce(&elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    return max_elapsed;
}

void printUsage(const char *argv0) {
    std::cout << "Usage: " << argv0
              << " [--engine omp|mpi|both] [--measurements N] [--batch-size N]"
                 " [--rules N] [--iterations N] [--output PATH]\n";
}

bool parseSize(const std::string &text, size_t &out) {
    try {
        size_t idx = 0;
        unsigned long long value = std::stoull(text, &idx, 10);
        if (idx != text.size()) {
            return false;
        }
        out = static_cast<size_t>(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseInt(const std::string &text, int &out) {
    try {
        size_t idx = 0;
        int value = std::stoi(text, &idx, 10);
        if (idx != text.size()) {
            return false;
        }
        out = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseArgs(int argc, char **argv, BenchmarkConfig &config, int rank) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            if (rank == 0) {
                printUsage(argv[0]);
            }
            return false;
        }
        if (arg == "--engine" && i + 1 < argc) {
            config.engine = argv[++i];
            continue;
        }
        if (arg == "--measurements" && i + 1 < argc) {
            if (!parseSize(argv[++i], config.measurements)) {
                if (rank == 0) {
                    std::cout << "Invalid measurements value.\n";
                }
                return false;
            }
            continue;
        }
        if (arg == "--batch-size" && i + 1 < argc) {
            if (!parseSize(argv[++i], config.batch_size)) {
                if (rank == 0) {
                    std::cout << "Invalid batch size value.\n";
                }
                return false;
            }
            continue;
        }
        if (arg == "--rules" && i + 1 < argc) {
            if (!parseSize(argv[++i], config.rules)) {
                if (rank == 0) {
                    std::cout << "Invalid rules value.\n";
                }
                return false;
            }
            continue;
        }
        if (arg == "--iterations" && i + 1 < argc) {
            if (!parseInt(argv[++i], config.iterations)) {
                if (rank == 0) {
                    std::cout << "Invalid iterations value.\n";
                }
                return false;
            }
            continue;
        }
        if (arg == "--output" && i + 1 < argc) {
            config.output_path = argv[++i];
            continue;
        }

        if (rank == 0) {
            std::cout << "Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
        }
        return false;
    }

    return true;
}

void printConfig(const BenchmarkConfig &config, int mpi_size, int rank) {
    if (rank != 0) {
        return;
    }

    std::cout << "Benchmark config\n";
    std::cout << "  engine: " << config.engine << "\n";
    std::cout << "  measurements: " << config.measurements << "\n";
    std::cout << "  batch size: " << config.batch_size << "\n";
    std::cout << "  rules: " << config.rules << "\n";
    std::cout << "  iterations: " << config.iterations << "\n";
    std::cout << "  output: " << config.output_path << "\n";
    std::cout << "  mpi ranks: " << mpi_size << "\n";
}

void printStats(const std::string &label, const Stats &stats, int rank) {
    if (rank != 0) {
        return;
    }
    std::cout << label << " avg " << stats.avg_ms << " ms, min " << stats.min_ms
              << " ms, max " << stats.max_ms << " ms\n";
}

void appendTimings(const std::string &path, const std::string &label,
                   const std::vector<double> &times_ms, int rank) {
    if (rank != 0 || times_ms.empty()) {
        return;
    }

    std::ofstream out(path, std::ios::app);
    if (!out) {
        std::cerr << "Failed to open output file: " << path << "\n";
        return;
    }

    for (double t : times_ms) {
        out << label << " " << t << "\n";
    }
}

} // namespace

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    BenchmarkConfig config;
    if (!parseArgs(argc, argv, config, rank)) {
        MPI_Finalize();
        return 0;
    }

    if (config.engine != "omp" && config.engine != "mpi" &&
        config.engine != "both") {
        if (rank == 0) {
            std::cout << "Engine must be one of: omp, mpi, both.\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (config.batch_size == 0 || config.measurements == 0 ||
        config.rules == 0 || config.iterations <= 0) {
        if (rank == 0) {
            std::cout << "Measurements, batch size, rules and iterations must "
                         "be positive.\n";
        }
        MPI_Finalize();
        return 1;
    }

    const size_t sensor_count =
        std::max<size_t>(1, std::min(config.rules, config.batch_size));
    const std::vector<std::string> sensors = makeSensorIds(sensor_count);
    const std::vector<TelemetryBatch> batches =
        makeBatches(config.measurements, config.batch_size, sensors);

    printConfig(config, size, rank);

    const bool run_omp = (config.engine == "omp" || config.engine == "both");
    const bool run_mpi = (config.engine == "mpi" || config.engine == "both");

    if (run_omp) {
        std::vector<double> times_ms;
        times_ms.reserve(config.iterations);

        for (int i = 0; i < config.iterations; ++i) {
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == 0) {
                times_ms.push_back(runOmpOnce(config, batches, sensors));
            }
            MPI_Barrier(MPI_COMM_WORLD);
        }

        if (rank == 0) {
            const Stats stats = computeStats(times_ms);
            printStats("RuleEngine", stats, rank);
            appendTimings(config.output_path, "RuleEngine", times_ms, rank);
        }
    }

    if (run_mpi) {
        std::vector<double> times_ms;
        times_ms.reserve(config.iterations);

        for (int i = 0; i < config.iterations; ++i) {
            const double elapsed =
                runMpiOnce(config, batches, sensors, MPI_COMM_WORLD);
            if (rank == 0) {
                times_ms.push_back(elapsed * 1000.0);
            }
        }

        if (rank == 0) {
            const Stats stats = computeStats(times_ms);
            printStats("MpiRuleEngine", stats, rank);
            appendTimings(config.output_path, "MpiRuleEngine", times_ms, rank);
        }
    }

    MPI_Finalize();
    return 0;
}

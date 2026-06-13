#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

#include "../src/AstraLog.h"

using namespace std::chrono_literals;

namespace {
// set of helper functions
std::string getTestsRoot() {
    return std::filesystem::path(__FILE__).parent_path().string();
}

std::string makeWorkDir(const std::string &prefix) {
    std::filesystem::path path =
        std::filesystem::path(getTestsRoot()) / (prefix);
    std::filesystem::create_directories(path);
    return path.string();
}

// we use this to copy the content of the collector_output
void copyCollectorOutput(const std::string &srcDir,
                         const std::string &destDir) {
    for (const auto &entry : std::filesystem::directory_iterator(srcDir)) {
        if (entry.path().extension() != ".txt") {
            continue;
        }

        std::filesystem::path destPath =
            std::filesystem::path(destDir) / entry.path().filename();
        std::filesystem::copy_file(
            entry.path(), destPath,
            std::filesystem::copy_options::overwrite_existing);
    }
}

size_t countTxtFiles(const std::string &dir) {
    size_t count = 0;
    for (const auto &entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".txt") {
            ++count;
        }
    }
    return count;
}

} // namespace

// RAII guard (Resource Acquisition Is Initialization) to ensure that the
// working directory is reset and the temporary directory is cleaned up after
// each test, even if exceptions occur. This is a common pattern in C++ to
// manage resources safely and avoid side effects between tests. In fact, this
// was added due to strange behaviour and side effects between these test.
struct ScopeCleanup {
    std::filesystem::path
        oldCwd; // store the original working directory to reset it later
    std::filesystem::path
        dir; // store the temporary directory to clean it up later
    explicit ScopeCleanup(const std::string &d)
        : oldCwd(std::filesystem::current_path()), dir(d) {
    } // constructor that takes the temporary directory path and stores the
      // current working directory
    ~ScopeCleanup() {
        std::error_code ec;
        std::filesystem::current_path(
            oldCwd, ec); // reset the working directory to the original one,
                         // handling any potential errors
        std::filesystem::remove_all(
            dir, ec); // remove the temporary directory and its contents,
                      // handling any potential errors
    }
};

TEST_CASE("AstraLog shuts down after having evaluated all batches in the "
          "collector directory",
          "[AstraLog][shutdown]") {
    const std::string testsRoot = getTestsRoot();
    const std::string fixturesDir =
        (std::filesystem::path(testsRoot) / "test_collector_output").string();
    const std::string rulesFile =
        (std::filesystem::path(testsRoot).parent_path() / "input" /
         "rules.json")
            .string();
    const std::string inputDir =
        makeWorkDir("test_fake_run"); // fake directory just for this test
    ScopeCleanup cleanup(inputDir);
    std::filesystem::current_path(inputDir);

    copyCollectorOutput(fixturesDir, inputDir);
    REQUIRE(countTxtFiles(inputDir) > 0);
    REQUIRE(std::filesystem::is_regular_file(rulesFile));

    AstraLog app(false, false, 16, 32);
    auto start = std::chrono::steady_clock::now();
    app.run(inputDir, rulesFile);
    auto elapsed = std::chrono::steady_clock::now() - start;

    REQUIRE(elapsed >= 5s);
    REQUIRE(elapsed < 12s);
}

TEST_CASE("AstraLog run removes ingested .txt files", "[AstraLog][cleanup]") {
    const std::string testsRoot = getTestsRoot();
    const std::string fixturesDir =
        (std::filesystem::path(testsRoot) / "test_collector_output").string();
    const std::string rulesFile =
        (std::filesystem::path(testsRoot).parent_path() / "input" /
         "rules.json")
            .string();
    const std::string inputDir = makeWorkDir("test_fake_cleanup");
    ScopeCleanup cleanup(inputDir);
    std::filesystem::current_path(inputDir);

    copyCollectorOutput(fixturesDir, inputDir);
    REQUIRE(
        countTxtFiles(inputDir) >
        0); // require that the content of the collector_output has been copied
    REQUIRE(std::filesystem::is_regular_file(rulesFile));

    AstraLog app(false, false, 16, 32);
    app.run(inputDir, rulesFile);

    REQUIRE(countTxtFiles(inputDir) ==
            0); // require that the number of files in the dir is 0
}

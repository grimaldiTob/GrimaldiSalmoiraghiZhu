#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "types/rules/SimpleRule.h"
#include <optional>

// ---------------------------------------------------------------------------
// Helper: centralise rule construction so every TEST_CASE uses the same rule
//   rule_id="rule1", priority=MEDIUM, sensor_id="temp", op=">", threshold=20.0
// ---------------------------------------------------------------------------
static SimpleRule makeRule() {
    return SimpleRule("rule1", RulePriority::MEDIUM, "temp", ">", 20.0);
}

// ===========================================================================
// Cases that produce a definite boolean result
// ===========================================================================

TEST_CASE("evaluate returns true when value is above threshold",
          "[SimpleRule][evaluate]") {
    SimpleRule rule = makeRule();

    SECTION("clearly above threshold") {
        std::optional<bool> result = rule.evaluate("temp,25.5");
        REQUIRE(result.has_value());
        CHECK(result.value() == true);
    }

    SECTION("just above threshold") {
        std::optional<bool> result = rule.evaluate("temp,21.0");
        REQUIRE(result.has_value());
        CHECK(result.value() == true);
    }
}

TEST_CASE("evaluate returns false when value is below threshold",
          "[SimpleRule][evaluate]") {
    SimpleRule rule = makeRule();

    std::optional<bool> result = rule.evaluate("temp,15.0");
    REQUIRE(result.has_value());
    CHECK(result.value() == false);
}

TEST_CASE("evaluate returns false when value equals threshold (strict >)",
          "[SimpleRule][evaluate]") {
    SimpleRule rule = makeRule();

    // 20.0 is NOT > 20.0, so the rule must be false
    std::optional<bool> result = rule.evaluate("temp,20.0");
    REQUIRE(result.has_value());
    CHECK(result.value() == false);
}

// ===========================================================================
// Cases that must return std::nullopt (not evaluated)
// ===========================================================================

TEST_CASE("evaluate returns nullopt for mismatched sensor id",
          "[SimpleRule][evaluate][nullopt]") {
    SimpleRule rule = makeRule();

    // "hum" != "temp" → sensor mismatch
    std::optional<bool> result = rule.evaluate("hum,25.5");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("evaluate returns nullopt for non-numeric value",
          "[SimpleRule][evaluate][nullopt]") {
    SimpleRule rule = makeRule();

    std::optional<bool> result = rule.evaluate("temp,abc");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("evaluate returns nullopt for missing delimiter",
          "[SimpleRule][evaluate][nullopt]") {
    SimpleRule rule = makeRule();

    // No comma → invalid format
    std::optional<bool> result = rule.evaluate("temp25.5");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("evaluate returns nullopt for extra fields",
          "[SimpleRule][evaluate][nullopt]") {
    SimpleRule rule = makeRule();

    std::optional<bool> result = rule.evaluate("temp,25.5,extra");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("evaluate returns nullopt for empty value field",
          "[SimpleRule][evaluate][nullopt]") {
    SimpleRule rule = makeRule();

    std::optional<bool> result = rule.evaluate("temp,");
    CHECK_FALSE(result.has_value());
}

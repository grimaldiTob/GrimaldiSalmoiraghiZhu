# Catch2 Essential Tools Cheatsheet

Instead of writing a custom main() for each test, we can use Catch2 as our testing framework.

It's already integrated in the CMakeLists.txt, so you just need to write TEST_CASE blocks and it handles everything else: test discovery, output formatting, and CTest registration.

I have put an pratcie example into the test folder (test_simple_rule_main_catch2.cpp) which replicate test_simple_rule_main.cpp using Catch2.

Remenber to add these following header before starting with code:
~~~c++
#include <catch2/catch_test_macros.hpp> // MANDATORY since it give you all essential test structures

#include <catch2/matchers/catch_matchers_string.hpp> // OPTIONAL for string matchers (Equals, StartsWith, Contains...)

#include <catch2/matchers/catch_matchers_floating_point.hpp> // OPTIONAL for float matchers (WithinAbs, WithinRel...)

#include <catch2/matchers/catch_matchers_range_equals.hpp> // OPTINAL for container/range matchers
~~~

## Test Structure

### `TEST_CASE`
The basic unit of testing. Takes a name and optional tags.
```cpp
TEST_CASE("description of what is being tested", "[tag1][tag2]")
{
    // test body
}
```

### `SECTION`
Subdivides a `TEST_CASE` into independent sub-scenarios. The `TEST_CASE` setup runs fresh for each `SECTION`.
```cpp
TEST_CASE("SimpleRule evaluate", "[SimpleRule]")
{
    SimpleRule rule("rule1", RulePriority::MEDIUM, "temp", ">", 20.0);

    SECTION("value above threshold")
    {
        CHECK(rule.evaluate("temp,25.5").value() == true);
    }
    SECTION("value below threshold")
    {
        CHECK(rule.evaluate("temp,15.0").value() == false);
    }
}
```

---

## Assertions

### `REQUIRE`
Fails the test **and stops execution immediately** if the condition is false. Use when subsequent code would crash or be meaningless without this condition being true.
```cpp
std::optional<bool> result = rule.evaluate("temp,25.5");
REQUIRE(result.has_value());       // stops here if nullopt
CHECK(result.value() == true);     // safe to call now
```

### `CHECK`
Fails the test but **continues execution**. Use for independent conditions where you want to see all failures at once.
```cpp
CHECK(result.value() == true);
CHECK(rule.getId() == "rule1");    // still runs even if the line above failed
```

### `REQUIRE_FALSE` / `CHECK_FALSE`
Negated versions — pass when the condition is `false`.
```cpp
std::optional<bool> result = rule.evaluate("hum,25.5");
CHECK_FALSE(result.has_value());   // passes if nullopt
```

---

## Exception Assertions

### `REQUIRE_THROWS` / `CHECK_THROWS`
Passes if the expression throws **any** exception.
```cpp
REQUIRE_THROWS(rule.evaluate(nullptr));
```

### `REQUIRE_THROWS_AS` / `CHECK_THROWS_AS`
Passes if the expression throws a specific exception type.
```cpp
REQUIRE_THROWS_AS(riskyCall(), std::invalid_argument);
```

### `REQUIRE_NOTHROW` / `CHECK_NOTHROW`
Passes if the expression throws **nothing**.
```cpp
REQUIRE_NOTHROW(rule.evaluate("temp,25.5"));
```

---

## Matchers
Matchers give richer failure messages than raw comparisons. Require `#include <catch2/matchers/...>`.

### String matchers
```cpp
#include <catch2/matchers/catch_matchers_string.hpp>

CHECK_THAT(rule.getId(), Catch::Matchers::Equals("rule1"));
CHECK_THAT(rule.getId(), Catch::Matchers::StartsWith("rule"));
CHECK_THAT(rule.getId(), Catch::Matchers::EndsWith("1"));
CHECK_THAT(rule.getId(), Catch::Matchers::ContainsSubstring("ule"));
```

### Floating-point matchers
```cpp
#include <catch2/matchers/catch_matchers_floating_point.hpp>

CHECK_THAT(result, Catch::Matchers::WithinAbs(20.0, 0.001));  // |result - 20.0| <= 0.001
CHECK_THAT(result, Catch::Matchers::WithinRel(20.0, 0.01));   // within 1% of 20.0
```

### Range / container matchers
```cpp
#include <catch2/matchers/catch_matchers_range_equals.hpp>

std::vector<int> v = {1, 2, 3};
CHECK_THAT(v, Catch::Matchers::RangeEquals(std::vector<int>{1, 2, 3}));
```

---

## Floating-Point Comparisons
Never use `==` for floats. Use the `Approx` helper or matchers above.
```cpp
CHECK(result == Catch::Approx(20.0));
CHECK(result == Catch::Approx(20.0).epsilon(0.01));  // 1% tolerance
CHECK(result == Catch::Approx(20.0).margin(0.001));  // absolute tolerance
```

---

## Logging & Info

### `INFO`
Adds a message to the failure output — only printed if a subsequent assertion fails.
```cpp
INFO("Testing input: " << input);
CHECK(rule.evaluate(input).has_value());
```

### `CAPTURE`
Shorthand to log the name and value of a variable on failure.
```cpp
CAPTURE(input);   // prints: input := "temp,25.5"
CHECK(rule.evaluate(input).has_value());
```

---

## Tags & Filtering
Run only a subset of tests from the command line using tags:
```bash
./test_simple_rule_catch2 "[evaluate]"          # run tests tagged [evaluate]
./test_simple_rule_catch2 "[evaluate][nullopt]"  # must match both tags
./test_simple_rule_catch2 "~[nullopt]"           # exclude [nullopt] tests
./test_simple_rule_catch2 --list-tests           # list all available tests
```

---

## Quick Reference Table

| Macro | Stops on fail? | Use for |
|---|---|---|
| `REQUIRE(expr)` | Yes | Preconditions, pointer/optional checks |
| `CHECK(expr)` | No | Independent value checks |
| `REQUIRE_FALSE(expr)` | Yes | Negated preconditions |
| `CHECK_FALSE(expr)` | No | Negated value checks |
| `REQUIRE_THROWS_AS(expr, type)` | Yes | Expected exceptions |
| `REQUIRE_NOTHROW(expr)` | Yes | Must not throw |
| `CHECK_THAT(val, matcher)` | No | Richer comparisons |
| `INFO(msg)` | — | Contextual debug info |
| `CAPTURE(var)` | — | Log variable on failure |

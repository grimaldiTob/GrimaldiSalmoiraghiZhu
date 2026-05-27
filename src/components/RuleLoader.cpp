#include <fstream>
#include <iostream>
#include <sstream>

#include "../types/rules/LogicalCorrelationRule.h"
#include "../types/rules/SimpleRule.h"
#include "../types/rules/StatefulRule.h"
#include "../types/rules/StepDifferenceRule.h"
#include "RuleLoader.h"

/** @brief Helper method which maps priority strings to the enum values.*/
RulePriority RuleLoader::parsePriority(std::string_view prio_str) {
    if (prio_str == "HIGH")
        return RulePriority::HIGH;
    if (prio_str == "MEDIUM")
        return RulePriority::MEDIUM;
    return RulePriority::LOW;
}

/** @brief Take the rules_list passed as an argument and sorts the value given
 * the priority attribute of the rule.
 */
void RuleLoader::sortRules(std::vector<std::shared_ptr<BaseRule>> &rules_list) {
    std::sort(rules_list.begin(), rules_list.end(),
              [](const auto &a, const auto &b) {
                  return a->getPriority() > b->getPriority();
              });
}

/**
 * Parses a JSON file and loads rules into the provided rules list.
 * @param parser The simdjson parser instance.
 * @param filename The name of the JSON file to parse.
 * @param rules_list The list to populate with parsed rules.
 */
void RuleLoader::loadRules(const std::string &filename,
                           std::vector<std::shared_ptr<BaseRule>> &rules_list) {
    // Read file contents into a string first to avoid relying on
    // simdjson::padded_string::load API
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::cerr << "Cannot load the file. Check the filename." << std::endl;
        return;
    }

    std::ostringstream ss;
    ss << ifs.rdbuf();
    std::string content = ss.str();
    simdjson::padded_string json(content);

    try {
        simdjson::ondemand::document doc = m_parser.iterate(json);

        for (simdjson::ondemand::object obj : doc.get_array()) {
            std::shared_ptr<BaseRule> current_rule;

            // Extract only the type, since it is needed to choose the right
            // parsing function. All other fields are parsed in the specific
            // parsing function.
            std::string_view rule_type_sv = obj["type"].get_string();
            std::string rule_type(rule_type_sv);

            if (rule_type == "simple") {
                current_rule = parseSimpleRule(obj);
            } else if (rule_type == "step_difference") {
                current_rule = parseStepDifferenceRule(obj);
            } else if (rule_type == "stateful") {
                current_rule = parseStatefulRule(obj);
            } else if (rule_type == "correlation") {
                current_rule = parseLogicalCorrelationRule(obj, rules_list);
            }

            if (current_rule) {
                rules_list.emplace_back(current_rule);
            }
        }

        sortRules(rules_list);
    } catch (const simdjson::simdjson_error &e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        // Leave rules_list empty on parse errors
        return;
    }
}

std::shared_ptr<BaseRule>
RuleLoader::parseSimpleRule(simdjson::ondemand::object &obj) {
    std::string rule_id(
        obj["rule_id"]
            .get_string()
            .value()); // The .value() method seems to be needed to convert from
                       // simdjson::ondemand::string to std::string
    double value = obj["value"].get_double();
    std::string sensor_id(obj["sensor_id"].get_string().value());
    std::string oprtor(obj["operator"].get_string().value());
    // If the piority string exists, parse it, otherwise default to LOW
    try {
        RulePriority priority =
            parsePriority(obj["priority"].get_string().value());
        return std::make_shared<SimpleRule>(rule_id, priority, sensor_id,
                                            oprtor, value);
    } catch (const std::exception &e) {
        // Defaul to LOW if there is any issue with the priority parsing (e.g.,
        // field missing or invalid value)
        return std::make_shared<SimpleRule>(rule_id, RulePriority::LOW,
                                            sensor_id, oprtor, value);
    }
}

std::shared_ptr<BaseRule>
RuleLoader::parseStepDifferenceRule(simdjson::ondemand::object &obj) {
    std::string rule_id(obj["rule_id"].get_string().value());
    std::string sensor_id(obj["sensor_id"].get_string().value());
    std::string oprtor(obj["operator"].get_string().value());
    double value = obj["value"].get_double();
    // If the piority string exists, parse it, otherwise default to LOW
    try {
        RulePriority priority =
            parsePriority(obj["priority"].get_string().value());
        return std::make_shared<StepDifferenceRule>(rule_id, priority,
                                                    sensor_id, oprtor, value);
    } catch (const std::exception &e) {
        // Defaul to LOW if there is any issue with the priority parsing (e.g.,
        // field missing or invalid value)
        return std::make_shared<StepDifferenceRule>(rule_id, RulePriority::LOW,
                                                    sensor_id, oprtor, value);
    }
}

std::shared_ptr<BaseRule>
RuleLoader::parseStatefulRule(simdjson::ondemand::object &obj) {
    std::string rule_id(obj["rule_id"].get_string().value());
    std::string sensor_id(obj["sensor_id"].get_string().value());
    std::string oprtor(obj["operator"].get_string().value());
    double value = obj["value"].get_double();
    double consecutive_meas = obj["consecutive_measurements"].get_double();
    // If the piority string exists, parse it, otherwise default to LOW
    try {
        RulePriority priority =
            parsePriority(obj["priority"].get_string().value());
        return std::make_shared<StatefulRule>(rule_id, priority, sensor_id,
                                              oprtor, consecutive_meas, value);
    } catch (const std::exception &e) {
        // Defaul to LOW if there is any issue with the priority parsing (e.g.,
        // field missing or invalid value)
        return std::make_shared<StatefulRule>(rule_id, RulePriority::LOW,
                                              sensor_id, oprtor,
                                              consecutive_meas, value);
    }
}

std::shared_ptr<BaseRule> RuleLoader::parseLogicalCorrelationRule(
    simdjson::ondemand::object &obj,
    const std::vector<std::shared_ptr<BaseRule>> &rules_list) {
    std::string rule_id(obj["rule_id"].get_string().value());
    std::string logic(obj["logic"].get_string().value());

    std::vector<std::shared_ptr<BaseRule>> corr_rules;
    for (auto id_val : obj["conditions"].get_array()) {
        std::string target_id(id_val.get_string().value());
        for (const auto &rule : rules_list) {
            if (rule->getRuleId() == target_id) {
                corr_rules.push_back(rule);
                break;
            }
        }
    }
    // If the piority string exists, parse it, otherwise default to LOW
    try {
        RulePriority priority =
            parsePriority(obj["priority"].get_string().value());
        return std::make_shared<LogicalCorrelationRule>(rule_id, priority,
                                                        logic, corr_rules);
    } catch (const std::exception &e) {
        // Default to LOW if there is any issue with the priority parsing (e.g.,
        // field missing or invalid value)
        return std::make_shared<LogicalCorrelationRule>(
            rule_id, RulePriority::LOW, logic, corr_rules);
    }
}
